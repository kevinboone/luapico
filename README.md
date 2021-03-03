# picolua #

A programming environment for Lua for the Raspberry Pi Pico
microcontroller.

## What is this? ##

`picolua` is a *proof-of-concept* Lua programming environment for the Pi
Pico. As well as the Lua run-time, it includes a rudimentary editor
and basic file management facilities. `picolua` is designed to be
operated by connecting the Pico's USB to a terminal emulator.
Lua functions have been added for controlling the Pico's GPIO
and other peripherals.
As a result, it is possible to enter and run simple programs that
manipulate connected devices, without the need for any particular
development tools.

`picolua` has some conceptual similarities with MicroPython on the
Pico. However, `picolua` is designed to be completely self-contained,
in that no addition tools (other than a terminal) are required to
create, edit, and test programs.

`picolua` maintains a filesystem in the Pico's flash ROM, that can store
multiple files, perhaps organized into directories.

Most of the standard Lua features are available, except those that
interact with an operating system. The Lua file handling routines
have been replaced by alternatives specific to the `picolua`
filesystem. 


## Building ##

`picolua` is implemented in C, and designed to be built using 
the documented method for the Pico C SDK. The method depends, for better
or worse, on CMake. Setting up CMake, the SDK, and the ARM compiler
toolchain, is documented in the Raspberry Pi documentation. I'm not a fan
of CMake, but building for the Pico is well-nigh impossible any other way.
If you have set up according to the documentation, you should be able
to build like this: 

    $ mkdir build_pico
    $ cd build_pico
    $ cmake .. 
    $ make

The build process is configured to provide maximal warning logging,
and will throw out thousands of messages, of varying degrees of scariness. 
The result should be a file `picolua.uf2`, that can be copied to the
Pico in bootloader mode.

For testing purposes, it should be possible to build a version that 
will work on a Linux workstation, like this:

    $ mkdir build_host
    $ cd build_host
    $ PICO_PLATFORM=host cmake ..
    $ make.

This should result in a `picolua` executable. The Linux version expects
to see a file at /tmp/picolua.blockdev whose size is at least 128kB.
This will be used to model the persistent storage that the Pico
version uses in flash. The Linux version is designed to model the
Pico version closely, including all its limitations. Of course, 
GPIO access and the like will not be available in this build.

## Notes about the Lua implementation ##

`picolua` is based on Lua 3.5. 

On start-up, `picolua` enters interactive mode. You can enter expressions,
which are evaluated and displayed, or run a program using `dofile` or
`require`. At the Lua prompt, you can invoke the (rudimentary) 
screen editor using
`edit "file.lua"`. One change from the conventional Lua interactive mode
is that if the result is a table, the contents are displayed. This makes
it possible to provide "commands" like `ls "/dir"` and `df()` -- these return
a table, which gets displayed.

Standard Lua has only one numeric data type (at least, one only one
that is accessible outside the Lua runtime). It can be set at compile
time, but then it's fixed. At present, it is set to `double` 
-- double-precision
floating-point. Many functions in the Pico interface of necessity take
integer arguments. A GPIO pin number, for example, can only be an
integer. Care needs to be take if using the results of calculations
as arguments to these functions. At the time of writing, the Pico
SDK includes double-precision floating-point support more-or-less
as a matter of course, so there's little to be gained by trying to
use lower precision.

The implementation uses one Lua context for the whole runtime.
So any functions defined, or variables set, by executing a Lua file
(e.g, by using `dofile "file.lua"`) remain in effect. This is nearly
always the expected behaviour.
 
As in regular Lua, a file can be loaded using
`dofile "file.lua"` and `require "file"`. There are subtle differences
between these methods, that are described in the Lua documentation,
and apply here too. Here, though, there's little need to use
`require`, because there isn't a file search path.

Like all Lua functions, the Pico functions can be used without brackets
when there is a single string argument. So it's correct to write
`edit "test.lua"` as well as `edit ("test.lua")`. Although these
functions are defined in a library called `pico`, and should probably
be written `pico.edit`, `pico.ls`, etc., they are also defined in the
global namespace as well, just for convenience.

Since there is no underlying shell, some of the Pico-specific functions
provide shell-like functionality. There are functions for editing
and copying files, for example, and querying system status.

## Interrupts ##

Sending Ctrl+C should interrupt a running program. The same key is used
to abandon a line in the line editor. Ctrl+C is not used to exit the
screen editor; in fact, it has no function there -- not even "copy".
See the Screen editor section for more information.

## Line editor ##

The line editor responds to cursor movement and backspace (delete on
some keyboards) keys. Ctrl+Left and Ctrl+Right move the cursor by
whole words. Ctrl+Home and Ctrl+End move the cursor to the start
and end of the line respectively 

Up-arrow and down-arrow scroll through a history
of previous lines. By default, the history is limited to the last
four lines. The line editor is nowhere near as sophisticated as that
found in most Linux shells -- but it's a lot smaller.

## Screen editor ##

The screen editor is fairly rudimentary, and designed specifically
to operate from a serial terminal. Some of the key bindings are not
very conventional, because I've tried to avoid key combinations 
that are taken over by terminal emulators and window managers.

The editor is invoked initially by running `edit "filename"`. 
Thereafter, further files can be opened by sending Ctrl+O and
entering a filename. If multiple files are open, you can
switch between them using Ctrl+E.

To get a list of key bindings, send Ctrl+@. It would have been
nice use Ctrl+H (for "help") but Ctrl+H usually generates a "backspace"
keycode.   

There are a few oddities that might be worth mentioning.

- The key combination Ctrl+\ executes the file in the editor as a Lua program. 

- The usual Ctrl+C for "copy" has been replaced by Ctrl-Y, because I wanted to reserve the Ctrl+C combination for an interrupt throughout the environment.

- If text is selected, the tab key indents the text two spaces. Shift-tab unindents by the same amount. If nothing is selected, the tab key simple enters a tab character.

- The editor auto-indents using spaces. At present, this behaviour can't be turned off

## Pico-specific functions ##

*adc_pin_init (pin)*

Set a specific GPIO pin for use as an analogue input. This only needs
to be done if the pin has previously been used for something else. 
This function does not enable the ADC, for the specified pin or
in any more general sense. 

*adc_select_input (input)*

Select which specific analog input `adc_get()` will read from. In 
practice, the `input` argument will be a number from 0..3 to select
pins 16-19. Input channel 4 is the internal temperature sensor. 
Note that the `input` argument is _not_ a pin number, but a channel
number, even though each channel (except the temperature sensor)
is assigned to a specific pin. Calling this function enables the ADC
subsystem, but it does not set a GPIO pin to be in an appropriate
state for analog input -- use `adc_pin_init()` for that.

*adc_get()*

Read an analog value from the currently-selected channel. The value
will be in the range 0..4095.

*cp ("path1", "path2")*

A crude file copier. `path1` refers to a file that exists; `path2`
is a file that may or may not exist. If it does exist, it is overwritten
without warning. This function cannot copy multiple files, or take
a directory name as either argument. 

*df*

Returns an array containing the total, used, and free space in the
persistent storage, in bytes.  

*edit "file"*

Invokes a (very) simple text editor on the file. Please note that
_this editor limits line length to 200 characters_.

*format ()*

*gpio_get (pin)*

Returns HIGH or LOW (0 or 1) according to the state of the input pin.

Formats the persistent storage, erasing any data. 

*gpio_pull_up (pin)*

Enables the built-in pull-up resistor for a particular pin. 

*gpio_put (pin,value)*

Set a GPIO output to HIGH or LOW (or 1 or 0). This method does not
implicitly make the pin and output -- use `gpio_set_dir()`.

*gpio_set_dir (pin, direction)*

Set a GPIO pin to GPIO\_IN or GPIO\_OUT (or 0 or 1).

*gpio_set_function (pin, function)*

Set the operating mode of a particular GPIO pin. To set "normal",
programmed I/O, use:

    gpio_set_function (pin, GPIO_FUNC_SIO)

There are also modes `GPIO_FUNC_I2C`, `GPIO_FUNC_PWM`, etc., that
must be used to select specific operating modes. 

*i2c_init (port, baud)*

Initialize a specific I2C port. For more information, see the section on I2C below.

*i2c_write_read (port, addr, output, input_length)*

Perform an I2C read, write, or read/write. 
For more information, see the section on I2C below.


*ls*
*ls "/directory"*

Returns an array containing the names of files and directories in the
specified directory. There is no way to tell, from these results alone,
whether the entries represent files or directories.
See the example `ll.lua` for an idea how to combine `pico.stat()` and
`pico.ls()` to implement a function like the Unix `ls -l`.

*pwm_pin_init (pin)*

Sets up a GPIO for hardware PWM operation. This function implicitly
sets the pin to PWM mode. The base frequency is the default provided
by the underlying C SDK. 

*pwm_pin_set_level (pin,level)*

Sets the PWM duty cycle for a specified pin, in the range 0 (off)
to 65535 (on). The pin must have been prepared by calling
`pwm_pin_init()` first.

*read "path"*

Reads the entire contents of the specified file into a string variable.
The file can contain zeros -- many (but not all) the Lua string-handling
functions work on files with embedded zeros. No value is returned, but
an exception is raised if the file cannot be read.

*readline()*

Reads a line from the terminal, and assigns it to a string variable. This
is broadly equivalent to the standard Lua `io.read`, except that it only
works on the terminal. If the user sends an interrupt (usually
Ctrl+C), the function raises an exception and stops further execution. If
the user sends end-of-message (usually Ctrl+D) it returns `nil`. Otherwise
it returns a string containing the line entered. 

*rm "path"*

Deletes a file or an empty directory. There is no return value, whether
is succeeds or fails. 

*sleep_ms (msec)*

Sleep for the specified number of milliseconds.

*stat "path"*

Returns a table containing the size and type of the specified file.
See the example `ll.lua` for an idea how to combine `pico.stat()` and
`pico.ls()` to implement a function like the Unix `ls -l`.

*write ("path", string)*

Writes a string variable to the specified file. No terminating zero is
written. An exception is raised if the file cannot be written.

*yrecv() or yrecv ("filename")*

Receives one or more files using the YModem protocol. See the
section below on YModem support for more details.

*ysend ("filename")*

Send a file using the YModem protocol. See the
section below on YModem support for more details.


## I2C support ##

The Pico has two I2C ports, that can be assigned to various pairs of
pins. `picolua` refers to these ports as '0' and '1'. 
To initialize the I2C subsystem for a specific port, use

    pico.i2c_init (port, baud_rate)

Baud rate is typically set to 400 000, although faster operation is
possible in good operating conditions.

It's necessary to set the specific pins to I2C operation:

    pico.gpio_set_function (pin, GPIO_FUNC_I2C)

Note that, although various pins can be set for I2C operation, there
is no choice about which pins correspond to which ports. Pins 16 and
17, for example, can only ever be used with port 0.

I2C requires that the bus lines be pulled up to a positive voltage at
one point. If this is not done anywhere else, it can be done in the
Pico using

    pico.gpio_pull_up (pin)

There is one function, `pico.i2c_write_read()` for both reads and writes.

    pico.i2c_write_read (port, addr, output_string, input_lenth)

The implementation in a single function makes it easy to perform the common
"combined" operation of a write followed without interruption by a read.

If there is no data to write, pass a zero-length string as the `output_string` argument. Lua allows binary data to be stored in strings, and it doesn't matter if some of the values are zero, For example, we can do:

    output_string = string.char (0xFF, 0x00,...)

The function must be told the number of bytes to read; this also can be
zero. If it is not zero, then `i2c_write_read()` returns a string
containing the specified number of bytes. This string can be 
unpacked using `string.byte()`. Don't forget that Lua indexes strings
starting at 1, not 0, by default.

For an example of I2C operation, see the `mpu6050.lua` example in the
source code bundle.  

## Analog-to-digital support ##

The Pico has a single ADC, which is multiplexed between five different
sources -- pins 16-19, and the internal temperature sensor.
The `pico.adc_select_input()` function selects which channel to use,
but be aware that it takes a channel number, not a pin number. 
On the other hand, the function `adc_pin_init()`, which prepares a 
pin for ADC operation, takes an actual pin number. These complication
follows from the fact that one of the ADC channels -- the temperature
sensor -- does not have a pin. So we can't just use pin numbers for
all the ADC functions. 

See the file `adctest.lua` in the source code bundle, for an example of
using the ADC.

## Hardware PWM outputs ##

`picolua` has rudimentary support for PWM outputs -- enough to control
the brightness of a lamp, for example. Although the Pico's PWN system
is very flexible -- and therefore complex -- `picolua` avoids this
complexity by using defaults for all settings. As a result, the only
functions needed are `pico.pwm_pin_init()` and `pico.pwm_pin_set_level()`.

See the file `led_fade.lua` in the source code bundle, for an example of
using hardware PWM.

## YModem suppport ##

`picolua` support the YModem protocol for sending and receiving files to
and from a
host system. The files can have any contents, but there is a limit
on a single file of 100kB -- this is just to protect the Pico filesystem
from a badly-behaved sender. In addition, files are sent by being 
buffered in memory first, so even 100kB might be too much.

`picolua` doesn't support the more common Xmodem protocol, because it's
not much use for anything other than ASCII text. Ymodem has the additional
advantage of allowing multiple files to be sent in one batch.

To start a receive operation, use `yrecv()` or `yrecv "filename"`. If
a filename is specified, it will take precedence over the filename
provided by the sender. However, it makes no sense to provide a filename
if the sender will be sending multiple files -- they'll just overwrite
one another. 

When using YModem from a terminal emulator, it's probably best to
start `yrecv` or `ysend` before starting the transfer in the terminal -- the
terminal might not allow you back to the prompt to run the command
after that. After starting either function, you'll probably see a row of
"C" characters -- that's the YModem start character. You should stop
seeing this as soon as communication starts. The terminal
will probably only allow a limited amount of time for `picolua` to
signal it is ready to receive, and `picolua` will 
only allow twenty seconds for the terminal to start sending. 

I've tested the YModem support with Minicom which, on Linux at least,
just invokes the command-line utilities `rb` and `rz` to do the
actual transfer. YModem is notoriously fussy, and I can't be sure
that any YModem utilities than these will work with `picolua`.

## Limitations and complications ##

### Terminal ###

Most of the testing has been done using the Minicom terminal
emulator. Whatever terminal is used, it *must* have the following
characteristics. It may be necessary to adjust the terminal settings,
or the screen window size, or both, as well as the terminal's internal
settings. All terminal emulators I'm aware of allow these settings to be adjusted; where possible, I've tried to make `picolua` work with Minicom's defaults.

- Most importantly, the terminal must have *exactly* 80 screen columns, and _at least_ 24 screen lines. Typically a Linux shell window has 25 lines by default, but Minicom uses one line for its status, so 24 are available. It won't hurt to have more than 24 lines, but it _will_ cause problems if there are more than 80 columns. `picolua` assumes the terminal is completely dumb, and won't attempt to work out from the terminal what its screen size is -- there is no robust way to do so, anyway.

- `Picolua` expects the terminal to wrap lines automatically. This is not the default with Minicom -- use `minicom -w`.

- The terminal must support basic ANSI/VT100 cursor movement and screen handling codes. 

- `Picolua` potentially uses _all_ the control key combinations, including Ctrl+[symbol]. Minicom, in particular, uses Ctrl+A to enter command mode, and this clashes with the editor's use of this key combination for "select all". In general, it might be necessary to reconfigure the terminal if there are too many clashes.

- The terminal should send ASCII code 8 for "backspace" and 128 for "delete". It should send code 13 (CR) when the "enter" key is pressed.

- The terminal should do a line-feed, not a carriage return, when it receives ASCII code 10.

If the terminal cannot be configured, it is possible to change most of these settings by editing `interface.h`.

## Line editor ##

The line editor allows lines up to 200 character long, but it does not
behave well once the line is longer than the terminal. This is because
terminals do not generally allow a backspace character to move the cursor
from the start of one line to the end of the previous line.  

### Operating system support ###

`picolua` runs without an operating system. The only file-handling functions 
available to Lua programs are those documented above, which read and write
whole files. In particular, there is no `io.write`, even for the terminal. So
there's no straightforward way to have a Lua program print a line of text
_without_ a newline at the end. The workaround is to build the whole line in a
string, and print that.

### Limited Pico hardware support ###

`picolua` supports general, polled GPIO operation for digital I/O, analog 
input, I2C read and write, and hardware PWM. It runs on a single core, 
leaving the other core idle. There is no support, and probably never 
will be, for DMA, interrupts, threading, or multi-core operation.  


## Acknowledgements ##

Lua was developed by, and continues to be maintained by, the Computer 
Science Department of the Pontifical Catholic University in Rio de Janeiro. 

The screen editor was derived from work by Michael Ringgaard.

The YModem implementation is based on work by Fredrik Hederstierna.

The LittleFS filesystem implementation was developed by ARM.

