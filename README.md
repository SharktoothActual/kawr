# kawr
**K**eep **A**live **W**hile **R**unning (or Keep Awake While Active, I don't really care what you call it)

### kawr is a small, lightweight tool designed to keep your system active while a program is running.

It's intended to be a featherweight alternative to `systemd-inhibit`. Right now, it doesn't let you give any reasons or set timers and nstead of logging things it just prints simple outputs directly to the console. It's designed to be very beginner friendly, as well. The reason I made this is because I like `systemd-inhibit` as a means of keeping my system active while `dd` runs, but I don't like the fact I have to give it a reason and specify a bunch of other crap. Not to mention that I, personally, don't care for `systemd` anyway and much prefer `OpenRC` (which of course doesn't have `systemd-inhibit`).

There are probably other programs out there that do the exact same thing, likely even better and were actually coded in languages the developers know. And that's great! You can use those! But this is more of a simple experiment to make a tool for my exact need in a language I don't know (C in this case, but really that'd be any of them as I don't code) as a means of learning. Of course, that means that **AI was involved in coding this project**. If you're the rational type who is still reading, you should be aware that I at least typed each line of code myself and tested it thoroughlly on my own system. I might not be able to code, but I can at least read it. **Naturally, due to the AI-assisted nature of this program I will happily accept any and all advice and improvments you have to offer.** Just keep in mind the intention of this program is to be small and maximally portable. It's supposed to be something the [suckless](https://suckless.org) guys would approve of. Not that they know me, anyway.

## Features
* Keeps your PC alive while a program is running (hence, the name)
* Does not rely on `systemd` (but it does need `dbus`)
* Works in both X and Wayland (and the TTY!)
* Doesn't let you run it as root, so it should be secure (unless you log in as root and have it installed system-wide)
* It should compile on musl (as well as glibc, naturally)
* Written by an all around chill dude (according to my mom)

## Installing
1) Clone the repo
2) Open a terminal and cd into the directory you cloned this repo into
3) Compile it using the following command:
```bash
gcc -O2 ./src/main.c -o kawr $(pkg-config --cflags --libs dbus-1)
```
That will drop the program into the root of the repo's directory.

**You can properly install it by doing either of the following:**
```bash
# For a single user
mkdir -p ~/.local/bin
mv kawr ~/.local/bin/

# For a system-wide installation
sudo mv kawr /usr/local/bin/
```

## Usage
Intentionally super simple. Use it like so:
```bash
# In a graphical environment
kawr mpv long_presentation.mp4

# Overriding to force console/terminal mode
kawr --console your_long_compile_script.sh
```

It also supports the following flags:
```
-g or --graphical = Forces kawr to run over dbus
-t or --terminal = Forces kawr to run in TTY mode using setterm
-c or --console = Same as above, included because I could
```

If you screw up the command by putting things out of order or trying to run it using `sudo` then the program should be smart enough to tell you.
```bash
# Running using privilege escalation
user (~/Git/kawr) $ sudo kawr firefox
[sudo] password for user:
You cannot run kawr using sudo or doas, as this can break your graphical display bus.
Correct usage: kawr sudo firefox

# Flag in the wrong spot
user (~/Git/kawr) $ kawr game -c
Inhibiting system while (game) is running.
INFO: If you meant to run kawr in (Graphical) mode, close game and run: "kawr -g game"
```
**Don't worry, it still passed through `-c` to `game`. It just gives you a heads-up in case you didn't mean to.**

You can exit out of the target program to close `kawr` or you can hit Ctrl+C to close it. Just bear in mind that closing kawr also closes the program it was running.

## License and Redistribution
No license provided. Let's just go with the old honor-system approach where I'll trust you to at least credit me if you include this in something. Also feel free to let me know if this program is useful for you, I'd be thrilled if others find it as useful as I do.

---
Last, but not least: A big thanks to anyone who wants to try this program out!