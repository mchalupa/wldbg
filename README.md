###  What is wldbg?

Wldbg is a tool that allows you to hijack wayland's connection
and do whatever you want with it. It can be easily extended
by passes or it can run in interactive gdb-like mode.

### What is pass?

The calling pass is inspired by LLVM passes.
Pass is a plugin that defines two functions - one for incoming
messages from client and the other for messages from server.
It can modify, print or take arbitrary action with the message and
then pass the message to another pass and so on..

### Using passes

Run wldbg with passes is very easy, just type on command-line:

```
  $ wldbg pass1 ARGUMENTS, pass2 ARGUMENTS -- wayland-client
```

pass1 and pass2 are names of the passes without .so extension
(i. e. dump.so ~~> dump)

If you do not know what passes are available, use:

```
  $ wldbg list
```

To get better understanding how it works, there's exapmle pass
in passes/example.c that is also compiled with other passes
in that directory. Run it as follows:

```
  $ wldbg example -- wayland_client
```

### Using interactive mode

To run wldbg in interactive mode, just do:

```
  $ wldbg -i wayland-client
```

If everything goes well, you'll see:

```
  Stopped on the first message
  C: wl_display@1.get_registry(new id wl_registry@2)
  (wldbg)
```

wldbg is now waiting for your input. By typing 'c' or 'continue', the program
will continue in running. Other usefull commands are:

```
'help'                    -- show help message
'help COMMAND'            -- show help to COMMAND
'b' or 'break'            -- stop running on specified messages
    b id 10                   --> stop on messages with id 10
    b re REGEXP               --> stop on any message matching regexp)
'i' or 'info'             -- show information about running state
    i b(reakpoints)           --> info about breakpoints
    i objects                 --> info about objects
    i proc                    --> info about process
'h' or 'hide              -- hide specified messages matching REGEXP, e. g.:
    h wl_pointer.*            --> hide everything for/from wl_pointer
    h wl_display@.*done       --> hide done messages from wl_display
'so' or 'showonly'        -- show only messages matching REGEXP
    so wl_display.*done       --> show only wl_display.done event
's' or 'send'             -- send message to client/server (in wire format)
'e' or 'edit'             -- edit message that we stopped at
'q' or 'quit'             -- exit wldbg
```

Ctrl-C interrupts the program and prompts user for input.


Wldbg is under hard (and slow :) developement and not all features are working yet
(those described above should mostly work, though)

Author: Marek Chalupa <mchqwerty@gmail.com>
