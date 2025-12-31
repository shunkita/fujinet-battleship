# Fuji Battleships
This is a WIP cross platform game client for the Fuji Battleships server.



### Supported Platforms
* **Apple II**
* **Atari**
* **CoCo**
* **C64** (WIP)
* **MS-Dos**
* *Please contribute to add more!*

### To Build
1. Review/edit the Post Build and the emulator start commands in `Makefile`
2. To build and test: `make [platform]`
3. Platforms: **apple2** **atari** **c64*** **coco*** **msdos**

### C64
To test in VICE, run support/c64/fuji_mock_network.py and make as follows:
* `make c64 VICE=1`

### CoCo
The distribution disk includes two binaries and a small loader to detect Coco 1/2 or 3 and run the appropriate binary. You may also build just one binary for testing.
* 	CoCo 1/2: 		`make coco`
* 	CoCo 3: 		`make coco3`
*   Combined Disk:  `make coco-dist`
*   Test Disk:      `make coco-dist test-coco-dist`

### Build Output - in /r2r

The "Ready 2 Run" output files will be in `./r2r`, which can be copied to a TNFS server, etc.

This project uses the MekkoGX Makefiles platform, which should automatically download the Fujinet-Lib dependency.


# Server / Api details

Please visit the server page for more information:

https://github.com/FujiNetWIFI/servers/tree/main/fujinet-game-system/battleship#readme
