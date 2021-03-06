-- Flash the on-board LED
gpio_pin = 25
pico.gpio_set_function (gpio_pin, GPIO_FUNC_SIO)
pico.gpio_set_dir (gpio_pin, GPIO_OUT)
while true do
  pico.gpio_put (gpio_pin, HIGH)
  pico.sleep_ms (300)
  pico.gpio_put (gpio_pin, LOW)
  pico.sleep_ms (300)
end

