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
2. Download from [Qt Creator Download link](https://www.qt.io/download-qt-installer)
3. Make the file ``qt-online-installer-linux-x64-4.10.0.run`` executable by entering from termianl in the directory:
   1) ``chmod +x qt-online-installer-linux-x64-4.10.0.run``.
   2) ``./qt-online-installer-linux-x64-4.10.0.run``
   3) Login with your Qt account
   4) Select Qt 6.7.x (or higher) and Qt Creator
   5) Choose the Desktop GCC 64-bit kit

### Arduino Firmware ###

#### Install Arduino IDE ####

1. ``sudo apt update``
2. ``sudo apt install arduino``

## Working with Qt Creator and Arduino ##

### Building and Running the Qt GUI


#### Open the Project ####

Either open it from the Qt Creator interface or go to the directory in terminal and use the command ``qtcreator .``. Make sure the ``.qrc`` file includes ``label_r.png``.

#### (IMPORTANT) Enabling Serial Port Support in Qt & Serial Debugging ####

1. Open and Login to the **Qt Maintenance Tool**
2. Select **Add or remove components**
3. Search and select **Qt Serial Port** and **Qt Serial Bus** under the **correct** Qt version
4. Save and exit the tool and restart Qt Creator

#### Serial Port Permission ####

This project requires serial port communication. Allow Qt to access the Arduino serial port (``/dev/ttyACM0`` for example): <br>
``sudo usermod -aG dialout $USER`` <br>
Then **log out and log back in** for the group change to take effect.

### Upload the Code to Arduino ###

1. Open the Arduino folder in Arduino IDE
2. Connect your board (e.g., Arduino Uno)
3. In Tools:
   - Board: Select your Arduino model (e.g., Uno)
   - Port: Select ``/dev/ttyACM0`` (or similar)
4. click **Upload**

Note: If upload fails due to permissions, you may also need: ``sudo chmod a+rw /dev/ttyACM0``

## Setting up and Running the System ##

### Hardware Notes ###

1. **X motor:** mounted for 59 cm travel range (~4214 steps at 1/32 microstepping)
2. **Y motor:** mounted for 28 cm travel range (~1992 steps)
3. **Limit switches** connected to ``homeXPin`` and ``homeYPin``
4. **Microstepping pins** configured via ``mode0``, ``mode1``, ``mode2``
5. Arduino **automatically homes** on power-up

### GUI Usage ###

1. Set **Spacing / Sample Time** in cm / seconds
2. Select **Scan Region** in grid (default: all selected)
3. **Click** ``Run Scan`` to start scanning (can be stopped by ``Stop Scan``)
4. **Positioning buttons** allow manual control:
   - ``X Forward``, ``X Backward``, ``Y Forward``, ``Y Backward``
   - ``Update Position``, ``Return Home``

### Scan Protocol ###

1. GUI sends command: ``<5, spacing, timing, rowMin, rowMax, colMin, colMax>`` (row and col are indices)
2. Arduino:
   - Auto-homes if scanner not at (0, 0)
   - Waits 2s for acquisition setup
   - Scans custom region with motor delay = timing
   - Auto-homes on completion
   - Sends ``<SCAN_DONE>`` back to GUI

## Debugging Arduino Through Terminal ##

One way to debug code on arduino is to bypass Qt Creator and use terminal with other dependencies. Here, we show a way to test serial connection and arduino responses (and outputs) through ``minicom``

1. Install ``minicom``
   - ``sudo apt update``
   - ``sudo apt install minicom``
2. Setting up ``minicom``
   - Enter setting using ``minicom -s``. In there, use arrows and enter keys for controlling.
   - Go to ``Serial port setup`` (use A~N to go to setting, enter to select). Change **serial device** to Arduino port (``/dev/ttyACM0`` for example), and change the **Bps/Par/Bits** setting to ``9600 8N1``.
   - ``Save setup as dfl`` and then ``Exit from Minicom``.
3. Connecting to Arduino
   - Enter ``minicom`` and the new setting should connect to Arduino automatically. The Arduino return-home initialization process should have the motor move in an L-shape.
4. Controlling and Communicating with Arduino
   - Note: minicom has bugs, and for me the commands I entered never displayed on the terminal side but is able to transfer to Arduino. Also, ``CTRL-A Z`` sometimes don't show the menu but it is still there for some reason.
   - Below are useful commands:
     1) ``<1, 0, 0>``: Step X Back
     2) ``<2, 0, 0>``: Step Y Back
     3) ``<3, 0, 0>``: Step X Forward
     4) ``<4, 0, 0>``: Step Y Forward
     5) ``<5, spacing, timing, rowMin, rowMax, colMin, colMax>``: Start Region Scan
        - ``spacing`` in cm
        - ``timing`` in seconds
        - ``rowMin``, ``rowMax``, ``colMin``, ``colMax`` are 0-indexed grid bounds
     6) ``<6, 0, 0>``: Stop Scan
        - Returns ``9`` if scan was running
        - Returns ``0`` isf scan was not active
     7) ``<7, x_cm, y_cm>``: Move to Specific Position (to ``(x_cm, y_cm)``)
     8) ``<8, 0, 0>``: Return to home ``(0, 0)``
     9) ``<9, i, 0>`` Enable (i=1) / Disable (i=0) Debug Mode
     10) ``<T, any, 0>`` Serial Port Test Command
   - To exit: ``CTRL-A + X + Enter``

## Developer Notes ##

- The system uses **serial markers** (``<...>``) for robust communication
- Debug messsages can be toggled via ``Debug Mode`` checkbox
- Estimated scan end time is displayed live (``--/--, --:--, --``)
