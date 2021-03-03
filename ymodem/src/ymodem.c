/*=========================================================================

  Ymodem

  ymodem/ymodem.c

  (c)2021 Kevin Boone, GPLv3.0

=========================================================================*/

#include <stdlib.h>
#include <stdint.h>
#include <interface/interface.h>
#include <config.h>
#include <storage/storage.h> 
#include <ymodem/ymodem.h>

#define YM_FILE_SIZE_LENGTH        (16)
#define FYMODEM_FILE_NAME_MAX_LENGTH  (64)

/* packet constants */
#define YM_PACKET_SEQNO_INDEX      (1)
#define YM_PACKET_SEQNO_COMP_INDEX (2)
#define YM_PACKET_HEADER           (3)      
#define YM_PACKET_TRAILER          (2)      
#define YM_PACKET_OVERHEAD         (YM_PACKET_HEADER + YM_PACKET_TRAILER)
#define YM_PACKET_SIZE             (128)
#define YM_PACKET_1K_SIZE          (1024)
#define YM_PACKET_RX_TIMEOUT_MS    (1000)
#define YM_PACKET_ERROR_MAX_NBR    (5)

/* contants defined by YModem protocol */
#define YM_SOH                     (0x01)  
#define YM_STX                     (0x02) 
#define YM_EOT                     (0x04) 
#define YM_ACK                     (0x06) 
#define YM_NAK                     (0x15) 
#define YM_CAN                     (0x18) 
#define YM_CRC                     (0x43) 
#define YM_ABT1                    (0x41) 
#define YM_ABT2                    (0x61) 

/*=========================================================================

  ymodem_crc16

=========================================================================*/
static uint16_t ymodem_crc16 (const uint8_t *buf, uint16_t len)
  {
  uint16_t x;
  uint16_t crc = 0;
  while (len--) 
    {
    x = (crc >> 8) ^ *buf++;
    x ^= x >> 4;
    crc = (crc << 8) ^ (x << 12) ^ (x << 5) ^ x;
    }
  return crc;
  }

/*=========================================================================

  ymodem_read_32

=========================================================================*/
static void ymodem_read_32 (const uint8_t* buf, uint32_t *val)
{
  const uint8_t *s = buf;
  uint32_t res = 0;
  uint8_t c;
  /* trim and strip leading spaces if any */
  do {
    c = *s++;
  } while (c == ' ');
  while ((c >= '0') && (c <= '9')) {
    c -= '0';
    res *= 10;
    res += c;
    /* next char */
    c = *s++;
  }
  *val = res;
}

/*=========================================================================

  ymodem_rx_packet

  Receive a single packet. Note that, in principle, successive packets
  can be of different size. This function sets the packet size into
  *rxlen in a successful receive. On a failure, *rxlen will be <= 0.

  Returns 0 on success, -1 on a timeout, 1 on a cancel or broken packet.
      -1: timeout or packet error
       1: abort by user / corrupt packet

=========================================================================*/
static int32_t ymodem_rx_packet (uint8_t *rxdata,
                            int32_t *rxlen,
                            uint32_t packets_rxed,
                            uint32_t timeout_ms)
  {
  *rxlen = 0;

  int32_t c = interface_get_char_timeout (timeout_ms);
  if (c < 0) 
    {
    /* end of stream */
    return -1;
    }

  uint32_t rx_packet_size;

  switch (c) 
    {
    case YM_SOH:
      rx_packet_size = YM_PACKET_SIZE;
      break;
    case YM_STX:
      rx_packet_size = YM_PACKET_1K_SIZE;
      break;
    case YM_EOT:
      /* ok */
      return 0;
    case YM_CAN:
      c = interface_get_char_timeout (timeout_ms);
      if (c == YM_CAN) 
        {
        *rxlen = -1;
        /* ok */
        return 0;
        }
      /* fall-through */
    case YM_CRC:
      if (packets_rxed == 0) 
        {
        /* could be start condition, first byte */
        return 1;
        }
     /* fall-through */
    case YM_ABT1:
    case YM_ABT2:
      /* User try abort, 'A' or 'a' received */
      return 1;
    default:
      /* This case could be the result of corruption on the first octet
         of the packet, but it's more likely that it's the user banging
         on the terminal trying to abort a transfer. Technically, the
         former case deserves a NAK, but for now we'll just treat this
         as an abort case. */
      *rxlen = -1;
      return 0;
    }

  /* store data RXed */
  *rxdata = (uint8_t)c;

  uint32_t i;
  for (i = 1; i < (rx_packet_size + YM_PACKET_OVERHEAD); i++) 
    {
    c = interface_get_char_timeout (timeout_ms);
    if (c < 0) 
      {
      /* end of stream */
      return -1;
      }
    /* store data RXed */
    rxdata[i] = (uint8_t)c;
    }

  /* just a sanity check on the sequence number/complement value.
     caller should check for in-order arrival. */
  uint8_t seq_nbr = (rxdata[YM_PACKET_SEQNO_INDEX] & 0xff);
  uint8_t seq_cmp = ((rxdata[YM_PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff);
  if (seq_nbr != seq_cmp) 
    {
    /* seq nbr error */
    return 1;
    }

  /* check CRC16 match */
  uint16_t crc16_val = ymodem_crc16 
     ((const unsigned char *)(rxdata + YM_PACKET_HEADER),
                                rx_packet_size + YM_PACKET_TRAILER);
  if (crc16_val) 
    {
    return 1;
    }

  *rxlen = rx_packet_size;
  /* success */
  return 0;
  }


/*=========================================================================

  ymodem_receive

=========================================================================*/
YmodemErr ymodem_receive (const char *out_filename, uint32_t maxsize)
  {
  YmodemErr err = 0;

  // Ensure we can write the output file, if specified. Although it
  //   will get created later, it's better to find out now, rather
  //   than in the middle of a long upload, that we can't write the
  //   file.
  if (out_filename)
    {
    if (storage_write_file (out_filename, "", 0) != 0)
      return YmodemWriteFile;
    }

  /* alloc 1k on stack, ok? */
  uint8_t rx_packet_data[YM_PACKET_1K_SIZE + YM_PACKET_OVERHEAD];
  int32_t rx_packet_len;

  uint8_t filesize_asc[YM_FILE_SIZE_LENGTH];
  uint32_t filesize = 0;

  BOOL first_try = TRUE;
  BOOL session_done = FALSE;

  char filename [MAX_FNAME]; 
  strcpy (filename, "untitled.txt");

  uint32_t nbr_errors = 0;

  /* z-term string */
  filename[0] = 0;
  
  /* receive files */
  do 
    { /* ! session done */
    if (first_try) 
      {
      /* initiate transfer */
      interface_write_char(YM_CRC);
      }
    first_try = FALSE;

    BOOL crc_nak = TRUE;
    BOOL file_done = FALSE;
    uint32_t packets_rxed = 0;

    uint32_t total_written = 0;
    do 
      { /* ! file_done */
      /* receive packets */
      int32_t res = ymodem_rx_packet (rx_packet_data,
                                 &rx_packet_len,
                                 packets_rxed,
                                 YM_PACKET_RX_TIMEOUT_MS);
      switch (res) 
        {
        case 0: 
          {
          /* packet received, clear packet error counter */
          nbr_errors = 0;
          switch (rx_packet_len) 
            {
            case -1: 
              {
              /* aborted by sender */
              interface_write_char (YM_ACK);
              return YmodemCancelled;
              }
            case 0: 
              {
              /* EOT - End Of Transmission */
              interface_write_char (YM_ACK);
              /* TODO: Add some sort of sanity check on the number of
                 packets received and the advertised file length. */
              file_done = TRUE;
              /* TODO: set first_try here to resend C ? */
              break;
              }
            default: 
              {
              /* normal packet, check seq nbr */
              uint8_t seq_nbr = rx_packet_data[YM_PACKET_SEQNO_INDEX];
              if (seq_nbr != (packets_rxed & 0xff)) 
                {
                /* wrong seq number */
                interface_write_char(YM_NAK);
                } 
              else 
                {
                if (packets_rxed == 0) 
                  {
              /* The spec suggests that the whole data section should
                 be zeroed, but some senders might not do this.
                 If we have a NULL filename and the first few digits of
                 the file length are zero, then call it empty. */
                  int32_t i;
                  for (i = YM_PACKET_HEADER; i < YM_PACKET_HEADER + 4; i++) 
                    {
                    if (rx_packet_data[i] != 0) 
                      {
                      break;
                      }
                    }
                  /* non-zero bytes found in header, filename packet has data */
                  if (i < YM_PACKET_HEADER + 4) 
                    {
                    /* read file name */
                    uint8_t *file_ptr = (uint8_t*)(rx_packet_data + YM_PACKET_HEADER);
                    i = 0;
                    while (*file_ptr && (i < MAX_FNAME)) 
                      {
                      filename[i++] = *file_ptr++;
                      }
                    filename[i++] = '\0';
                    file_ptr++;
                    /* read file size */
                    i = 0;
                    while ((*file_ptr != ' ') && (i < YM_FILE_SIZE_LENGTH)) 
                      {
                      filesize_asc[i++] = *file_ptr++;
                      }
                    filesize_asc[i++] = '\0';
                    /* convert file size */
                    ymodem_read_32 (filesize_asc, &filesize);
                    /* check file size */
                    if (filesize > maxsize) 
                      {
		      err = YmodemTooBig;
                      goto rx_err_handler;
                      }
                    interface_write_char (YM_ACK);
                    interface_write_char (crc_nak ? YM_CRC : YM_NAK);
                    crc_nak = FALSE;
                    }
                  else 
                    {
                    /* filename packet is empty, end session */
                    // TODO -- carry on if no filename
                    interface_write_char (YM_ACK);
                    file_done = TRUE;
                    session_done = TRUE;
                    break;
                    }
                  }
                else 
                  {
                  int to_write = rx_packet_len;
                  if (total_written + rx_packet_len > filesize)
                    to_write = filesize - total_written;
		  const char *current_filename;
		  if (out_filename)
		    current_filename = out_filename;
		  else
		    current_filename = filename;
                  err = storage_append_file (current_filename, 
                    rx_packet_data + YM_PACKET_HEADER, to_write); 
		  if (err) goto rx_err_handler;
                  interface_write_char (YM_ACK);
                  total_written += rx_packet_len;
                  }
                packets_rxed++;
                }  /* sequence number check ok */
              } /* default */
            } /* inner switch */
          break;
          } /* case 0 */
        default: 
	  {
          /* ym_rx_packet() returned error */
          if (packets_rxed > 0) 
	    {
            nbr_errors++;
            if (nbr_errors >= YM_PACKET_ERROR_MAX_NBR) 
	      {
	      err = YmodemBadPacket;
              goto rx_err_handler;
              }
            }
          interface_write_char(YM_CRC);
          break;
          } /* default */
        } /* switch */
      
      /* check end of receive packets */
      } while (! file_done);

    /* check end of receive files */
    } while (! session_done);

  return 0;

  rx_err_handler:
 
  interface_write_char(YM_CAN);
  interface_write_char(YM_CAN);
  interface_sleep_ms (1000);
  
  return err;
  }

/*=========================================================================

  ymodem_writeU32

  // TODO: can we just use sprintf?

=========================================================================*/
static uint32_t ymodem_writeU32 (uint32_t val, uint8_t *buf)
  {
  uint32_t ci = 0;
  if (val == 0) 
    {
    /* If already zero then just return zero */
    buf[ci++] = '0';
    }
  else 
    {
    uint8_t s[11];
    int32_t i = sizeof(s) - 1;
    /* z-terminate string */
    s[i] = 0;
    while ((val > 0) && (i > 0)) 
      {
      /* write decimal char */
      s[--i] = (val % 10) + '0';
      val /= 10;
      }
    
    uint8_t *sp = &s[i];
    
    /* copy results to out buffer */
    while (*sp) 
      {
      buf[ci++] = *sp++;
      }
    }
  /* z-term */
  buf[ci] = 0;
  /* return chars written */
  return ci;
  }

/*=========================================================================

  ymodem_send_packet

=========================================================================*/
static void ymodem_send_packet (uint8_t *txdata, int32_t block_nbr)
  {
  int32_t tx_packet_size;

  /* We use a short packet for block 0, all others are 1K */
  if (block_nbr == 0) 
    {
    tx_packet_size = YM_PACKET_SIZE;
    }
  else 
    {
    tx_packet_size = YM_PACKET_1K_SIZE;
    }

  uint16_t crc16_val = ymodem_crc16 (txdata, tx_packet_size);

  /* For 128 byte packets use SOH, for 1K use STX */
  interface_write_char ( (block_nbr == 0) ? YM_SOH : YM_STX );
  /* write seq numbers */
  interface_write_char (block_nbr & 0xFF);
  interface_write_char (~block_nbr & 0xFF);

  /* write txdata */
  int32_t i;
  for (i = 0; i < tx_packet_size; i++) 
    {
    interface_write_char (txdata[i]);
    }

  /* write crc16 */
  interface_write_char ((crc16_val >> 8) & 0xFF);
  interface_write_char (crc16_val & 0xFF);
  }

/*=========================================================================

  ymodem_send_packet0

=========================================================================*/
static void ymodem_send_packet0 (const char* filename,
                            int32_t filesize)
  {
  int32_t pos = 0;
  /* put 256byte on stack, ok? reuse other stack mem? */
  uint8_t block [YM_PACKET_SIZE];
  if (filename) 
    {
    /* write filename */
    while (*filename && (pos < YM_PACKET_SIZE - YM_FILE_SIZE_LENGTH - 2)) 
      {
      block[pos++] = *filename++;
      }
    /* z-term filename */
    block[pos++] = 0;
    
    /* write size, TODO: check if buffer can overwritten here. */
    pos += ymodem_writeU32 (filesize, &block[pos]);
    }

  /* z-terminate string, pad with zeros */
  while (pos < YM_PACKET_SIZE) 
    {
    block[pos++] = 0;
    }
  
  /* send header block */
  ymodem_send_packet (block, 0);
  }

/*=========================================================================

  ymodem_send_data_packets

=========================================================================*/
static void ymodem_send_data_packets (uint8_t* txdata,
                                 uint32_t txlen,
                                 uint32_t timeout_ms)
  {
  int32_t block_nbr = 1;
  
  while (txlen > 0) 
    {
    /* check if send full 1k packet */
    uint32_t send_size;
    if (txlen > YM_PACKET_1K_SIZE) 
      {
      send_size = YM_PACKET_1K_SIZE;
      } 
    else 
      {
      send_size = txlen;
      }
    /* send packet */
    ymodem_send_packet (txdata, block_nbr);
    int32_t c = interface_get_char_timeout (timeout_ms);
    switch (c) 
      {
      case YM_ACK: 
        {
        txdata += send_size;
        txlen -= send_size;
        block_nbr++;
        break;
        }
      case -1:
      case YM_CAN: 
        {
        return;
        }
      default:
        break;
      }
    }
  
  int32_t ch;
  do 
    {
    interface_write_char (YM_EOT);
    ch = interface_get_char_timeout (timeout_ms);
    } while ((ch != YM_ACK) && (ch != -1));
  
  /* send last data packet */
  if (ch == YM_ACK) 
    {
    ch = interface_get_char_timeout(timeout_ms);
    if (ch == YM_CRC) 
      {
      do 
        {
        ymodem_send_packet0 (0, 0);
        ch = interface_get_char_timeout(timeout_ms);
        } while ((ch != YM_ACK) && (ch != -1));
      }
    }
  }

/*=========================================================================

  ymodem_send_data

=========================================================================*/
YmodemErr ymodem_send_data (uint8_t *txdata, uint32_t txsize, 
          const char *filename)
  {
  YmodemErr err = 0;
  /* not in the specs, send CRC here just for balance */
  int32_t ch = YM_CRC;
  do 
    {
    interface_write_char (YM_CRC);
    ch = interface_get_char_timeout (1000);
    } while (ch < 0);

  /* we do require transfer with CRC */
  if (ch != YM_CRC) 
    {
    err = YmodemNoCRC;
    goto tx_err_handler;
    } 

  bool crc_nak = true;
  bool file_done = false;
  do 
    {
    ymodem_send_packet0 (filename, txsize);
    /* When the receiving program receives this block and successfully
       opened the output file, it shall acknowledge this block with an ACK
       character and then proceed with a normal XMODEM file transfer
       beginning with a "C" or NAK tranmsitted by the receiver. */
    ch = interface_get_char_timeout (YM_PACKET_RX_TIMEOUT_MS);

    if (ch == YM_ACK) 
      {
      ch = interface_get_char_timeout (YM_PACKET_RX_TIMEOUT_MS);

      if (ch == YM_CRC) 
        {
	storage_write_file ("foo.txt", "hello\n", 6);
        ymodem_send_data_packets (txdata, txsize, YM_PACKET_RX_TIMEOUT_MS);
        /* success */
        file_done = true;
        }
      else if (ch == YM_CAN)
        {
	err = YmodemCancelled;
	goto tx_err_handler;
	}
      }
    else if ((ch == YM_CRC) && (crc_nak)) 
      {
      crc_nak = false;
      continue;
      }
    else if ((ch != YM_NAK) || (crc_nak)) 
      {
      err = YmodemBadPacket;
      goto tx_err_handler;
      }
    } while (!file_done);

  return 0;

  tx_err_handler:

  interface_write_char (YM_CAN);
  interface_write_char (YM_CAN);
  interface_sleep_ms (1000);
  return err;
  }

/*=========================================================================

  ymodem_send

=========================================================================*/
YmodemErr ymodem_send (const char *filename)
  {
  YmodemErr err = 0;
  uint8_t *buff;
  int size;
  if (storage_read_file (filename, &buff, &size) == 0)
    {
    // TODO -- split basename off filename
    err = ymodem_send_data (buff, size, filename);
    free (buff);
    }
  else
    err = YmodemReadFile;
  return err;
  }

/*=========================================================================

  ymodem_strerror

=========================================================================*/
const char *ymodem_strerror (YmodemErr err)
  {
  switch (err)
    {
    case YmodemOK: return "OK";
    case YmodemWriteFile: return "Can't write file";
    case YmodemReadFile: return "Can't read file";
    case YmodemTooBig: return "File too large";
    case YmodemChecksum: return "Bad checksum";
    case YmodemCancelled: return "Transfer cancelled";
    case YmodemBadPacket: return "Corrupt packet";
    case YmodemNoCRC: return "Sender does not support CRC";
    }
  return "OK";
  }

