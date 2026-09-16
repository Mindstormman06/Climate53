# Climate53 - A TotK mod
This is a mod for `The Legend of Zelda: Tears of the Kingdom` that changes the climate of the map to be Death Mountain heat with constant thunderstorms. Built specifically to test adding new climates.

# Building .wxlm

## 1. Verify Python install

```
python --version
```

## 2. Install devkitPPC

Get the installer from [devkitpro.org](https://devkitpro.org/wiki/Getting_Started)
and select devkitPPC. It covers Cemu and Wii U, which build the same module.

Building for Switch too? Also install devkitA64.

Check it:

```
C:\devkitPro\devkitPPC\bin\powerpc-eabi-g++ --version
```

If devkitPro is somewhere else, set `DEVKITPPC` to that directory.

## 3. WiiXLaunch SDK

Download and extract the [WiiXLaunch SDK](https://github.com/BladesawStudios/WiiXLaunch/releases)

## 4. Build the mod
Open a terminal in your SDK folder.
```bash
python scripts\build_mod.py --source <path>/<to>/Climate53/
```

# Installing the mod

## 1. Clone WiiXLaunch
```bash
git clone https://github.com/BladesawStudios/WiiXLaunch.git
```

## 2. Build the host
Open a command prompt inside the WiiXL folder and run:
```bash
build_switch.bat totk
python scripts/deploy.py --target totk
```

## 3. Install host
You will find your built host under 
```
deploy/switch/atmosphere/contents/01007EF00011E000/exefs/
``` 
Copy the `exefs` folder into `<emulator-mods>/Climate53>

## 4. Install mod
Copy your previously built `.wxlm` to
```
<emulator>/sdcard/WiiXLaunch/mods/0100F2C0115B6000/
```

## Done
Enable `WiiXLaunch` in your emulator and you're good to go!