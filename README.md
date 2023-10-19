###  What is wldbg?

Wldbg is a tool that allows you to debug or modify events in
Wayland connections. It works on the man-in-the-middle basis.
When a Wayland client is run under wldbg, every message to or
from the client is handed out to a pipeline of passes that can
process the message. Passes are simple-to-write plugins.
Wldbg has also an interactive gdb-like mode.

### What is a pass?

The passes in wldbg are inspired by LLVM passes.
A pass is a plugin that defines two functions - one for
messages going from the client to the server and the another for
messages from server to the client.
It can analyze or arbitriraly modify the message and
then pass the message to the next pass (or stop the whole pipeline)
and so on..

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

To get better understanding how it works, there's an example pass
in `passes/example.c`. This pass is also compiled by default, so
you can try it out as follows:

```
  $ wldbg example -- wayland_client
```

### Using the interactive mode

To run wldbg in the interactive mode, just do:

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
will continue running. Other usefull commands are:

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
'autocmd'                 -- run command after messages of intereset
    autocmd add RE CMD        --> run CMD on every message matching RE
    autocmd add '' i o        --> display info about objects after every message
'h' or 'hide              -- hide specified messages matching REGEXP, e. g.:
    h wl_pointer.*            --> hide everything for/from wl_pointer
    h wl_display@.*done       --> hide done messages from wl_display
'so' or 'showonly'        -- show only messages matching REGEXP
    so wl_display.*done       --> show only wl_display.done event
's' or 'send'             -- send message to client/server (in wire format)
'e' or 'edit'             -- edit message that we stopped at
'q' or 'quit'             -- exit wldbg
```

When wldbg is run with -g (-objinfo) option, it gathers information about objects.
User then can just type:

```
(wldbg) i o ID
```

where ID is the id of the object of interest and wldbg will dump information
it gathered about it. NOTE: this is new and incomplete feature. At this
moment wldbg gathers information about xdg_surface, wl_surface and wl_buffer objects.

Ctrl-C interrupts the program and prompts user for input.

### Using server mode

Wldbg can run in the server mode in which every new connection is redirected to wldbg and
only then to the Wayland compositor. Upon the first connection, the user is prompted for an action,
every other connection is then connected automatically without stopping.
The server mode is interactive, so everything that works in the interactive mode,
works in the server mode too. To start wldbg in the server mode, use the -s switch:

```
$ wldbg -s
Listening for incoming connections...
```

when client connects, you'll see something like:

```
Stopped on the first message
[weston-terminal: 7874] C: wl_display@1.get_registry(new id wl_registry@2)
```

After another client is connected, the messages from both are just interleaved:

```
[weston-terminal |7874] S: wl_pointer@3.motion(2739198098, 197.000000, 142.000000)
[weston-terminal |7874] S: wl_pointer@3.frame()
[weston-dnd      |7930] S: wl_buffer@38.release()
[weston-dnd      |7930] S: wl_pointer@3.leave(166, wl_surface@14)
[weston-dnd      |7930] S: wl_pointer@3.frame()
[weston-terminal |7874] C: wl_surface@10.attach(wl_buffer@17, 0, 0)
[weston-terminal |7874] C: wl_surface@10.damage(0, 0, 9, 16)
[weston-terminal |7874] C: wl_surface@10.commit()
```

Server mode is handy, for example, for debugging the interaction between two clients,
like two weston-dnd instances dragging and dropping between them.

----------------------

An active development of Wldbg stopped some years ago, but it still should work.
There are features missing and probably some bugs, but the features described above
should mostly work.

Author: Marek Chalupa <mchqwerty@gmail.com>
