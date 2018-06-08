# RA3Bar-RA3Launcher
## Features
* Start Red Alert 3 nearly instantly
* You can now click Red Alert 3's splash screen to open RA3's Control Center. This could be pretty handy if you wish to play Mods(*) with current version C&C:Online Launcher.
* You can now add custom command-line arguments directly inside the Control Center. Again, this could be pretty handy if you want to add some command line arguments when using C&C:Online Launcher.
* Control Center's Game Browser is now equipped with a simple replay parser which can display some match information such as date, game duration, map, and player list. It can also fix corrupted replays damaged by game crash.
* If you can't open (and watch) \*.RA3Replay files with RA3.exe, now you can set the replay file association in a rather easy way: right click on the replay file, and choose (this new) RA3.exe in the **Open With...** menu. Then you can watch replays by just double-clicking on the file.

(*) In case you don't know how to load Mods: you need go to Control Center → Game Browser → Mod. To install a Mod, put them in `My Documents\Red Alert 3\Mods`.

## About this program
Recently a lot of people needs to wait for like 30 seconds when launching Red Alert 3.

This is because Red Alert 3's game launcher, **RA3.exe**, will firstly check "Comrade News"(*) before starting the main game process. But EA recently shut down the server files.ea.com, and RA3.exe would need to wait until the connection times out, before starting the game.

While this problem can be "fixed" in some rather simple way, such as by cutting off the Internet connection when launching the game (so the connection to EA server would fail instantly instead of waiting 30 seconds), or by editing the host file, I thought I could just write a my own RA3.exe. After all, RA3.exe is just a game launcher, isn't it?
It looks like that I just need write a simple program which calls `CreateProcess()` to start game's main process, it looks like to be a really easy task!

So I started this project by using just Win32 API because I thought it would be enough, and eventually I discovered that... the things aren't that easy like what I thought...
For example, actually I have to replicate game's Control Center with all GUI stuff, I have to parse replays, I have to parse CSF files and read localized strings from them... And I'm not very good at coding (you can confirm this by reading my code xD), and with those old Win32 API together, this project nearly became a big pain, but fortunately I still completed it!

(*) Example link of Comrade News: http://files.ea.com/downloads/eagames/cc/tiberium/RedAlert3/RA3ComradeNews/RA3ComradeNews_english.html
