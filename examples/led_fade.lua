-- Fade an LED up and down using hardware PWM

pin=25
pico.pwm_pin_init (pin)

function fade_up()
  local i
  for i=0,65535,4
  do
    pico.pwm_pin_set_level (pin, i);
  end
end

function fade_down()
  local i
  for i=65535,0,-4
  do
    pico.pwm_pin_set_level (pin, i);
  end
end

for i=0,4
do
  fade_up()
  fade_down()
end

