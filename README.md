# 2D Stepper Controller #

---

**Developed by:** Ci-You Huang and Josh Burdine  
**Principal Investigator:** Dr. Chen-Yu Liu  
**Graduate Mentor:** Nathan Callahan  

This code was developed for the **UCNτ project** conducted at the **University of Illinois Urbana-Champaign** and **Los Alamos National Laboratory**, as well as other affiliated research facilities.

Use of this code is permitted **with proper reference** to the original authors and the UCNτ project.

**Version:** June 14, 2025 — 18:41 PM

## Overview ##

This project controls a **2D stepper motor scanning system** using:
- A **Qt-based GUI** for scan configuration, live control, and serial communication
- An **Arduino firmware** to manage stepper motor movement, home positioning, and scanning

## Folder Structure ##

stepper-controller/            ← Root of the GitHub repo <br>
├── UCN_Scanner_V3/            ← Qt Creator GUI project <br>
│   ├── mainwindow.h/.cpp      ← MainWindow class definition and implementation <br>
│   ├── UCN_Scanner_V3.pro     ← Qt project file <br>
│   └── ...                    ← Other UI and resource files <br>
│ <br>
├── stepper_control_GUI_Ver2/  ← Arduino sketch controlling the stepper hardware <br>
│   ├── stepper_control.ino    ← Arduino firmware (pins, scanning logic, serial comms) <br>
│ <br>
├── .gitignore                 <br>
│ <br>
└── README.md                  ← This file — user & developer guide <br>

## Software Installation (Linux x86-64 System) ##

### Qt GUI and Qt Creator ###

To build and run the GUI for the 2D stepper system, you need **Qt 6.7+** installed. The free **education version** of the Qt Creator works perfectly. For other os, adapt this process accordingly. 

#### Installing Qt Creator (Education Version)

1. Fill out the form and follow the instruction on [Qt Creator Download Instruction](https://www.qt.io/qt-educational-license) to register an education account.
2. Download from [Qt Creator Download link](https://www.gt.io/download-gt-installer)
3. Make the file ``qt-online-installer-linux-x64-4.10.0.run`` executable by entering from termianl in the directory:
   1) ``chmod +x qt-online-installer-linux-x64-4.10.0.run``.
   2) ``./qt-online-installer-linux-x64-4.10.0.run``
   3) Login with your Qt account
   4) Select Qt 6.7.x (or higher) and Qt Creator
   5) Choose the Desktop GCC 64-bit kit

#### Open the Project ####

Either open it from the Qt Creator interface or go to the directory in terminal and use the command ``qtcreator .``.

#### Serial Port Permission ####

This project requires serial port communication. Allow Qt to access the Arduino serial port (``/dev/ttyACM0`` for example): <br>
``sudo usermod -aG dialout $USER`` <br>
Then **log out and log back in** for the group change to take effect.

### Arduino Firmware ###

#### Install Arduino IDE ####

1. ``sudo apt update``
2. ``sudo apt install arduino``

#### Upload the Code to Arduino ####

1. Open the Arduino folder in Arduino IDE
2. Connect your board (e.g., Arduino Uno)
3. In Tools:
   - Board: Select your Arduino model (e.g., Uno)
   - Port: Select ``/dev/ttyACM0`` (or similar)
4. click **Upload**

Note: If upload fails due to permissions, you may also need: ``sudo chmod a+rw /dev/ttyACM0``

## Working with Qt Creator and Arduino ##


## Setting up and Running the System ##
