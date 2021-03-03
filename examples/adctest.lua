-- Reads an analog input on pin 26 (a.k.a channel 0)
pin=26
MAX_ADC=4096
pico.adc_pin_init (pin)
pico.adc_select_input (pin - 26)
while true do
  local v = pico.adc_get() / MAX_ADC
  print (v)
  pico.sleep_ms (200) 
end

