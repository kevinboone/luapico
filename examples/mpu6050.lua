-- Read some data from an MPU6050 I2C accelerometer. See the pin
-- assignements below for connections. It may also be necessary
-- to change the I2C port number
 
sda_pin = 16
scl_pin = 17
i2c_port = 0
i2c_addr = 0x68
 
-- I2C codes for the MPU6050, from the datasheet
 
RESET_CODE = string.char (0x6B, 0x00)
GET_TEMP_CODE = string.char (0x41)
GET_GYRO_CODE = string.char (0x43)
 
-- convert16 converts the two bytes from I2C, which represent a 16-bit
-- signed number, into a decimal number
 
function convert16 (b1, b2)
  local v  = b1 * 256 + b2;
  if (v > 32767) then v = v - 65536 end;
  return v
end
-- Initialize the Pico I2C and the MPU device
 
function init ()
  pico.i2c_init (i2c_port, 400 * 1000)
  pico.gpio_set_function (sda_pin, GPIO_FUNC_I2C);
  pico.gpio_set_function (scl_pin, GPIO_FUNC_I2C);
  pico.gpio_pull_up (sda_pin)
  pico.gpio_pull_up (scl_pin)
  -- Reset the MPU6050
  pico.i2c_write_read (i2c_port, i2c_addr, RESET_CODE, 0)
end
 
function get_one()
  
  -- Get temperature
  local res = pico.i2c_write_read (i2c_port, i2c_addr, GET_TEMP_CODE, 2)
  
  t = convert16 (string.byte(res, 1), string.byte(res, 2));
  -- This conversion is from the datasheet (but it needs calibrating)
  local temp = (t / 340) + 36.53
  
  -- Get gyros
  res = pico.i2c_write_read (i2c_port, i2c_addr, GET_GYRO_CODE, 6)
  local x = convert16 (string.byte(res, 1), string.byte(res, 2));
  local y = convert16 (string.byte(res, 3), string.byte(res, 4));
  local z = convert16 (string.byte(res, 5), string.byte(res, 6));
  
  print ("temp", temp, "x", x, "y", y, "z", z)
end

-- Start here
 
init()
 
while true do
  get_one()
  sleep_ms (500)
end
 

