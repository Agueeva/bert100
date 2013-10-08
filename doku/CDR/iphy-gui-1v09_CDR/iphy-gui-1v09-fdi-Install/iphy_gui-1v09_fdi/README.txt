README -
8/11 - Initializes with startup_gb.py(version 2 - Aug 2, 2012)
8/11 - Create gen25 - enables both gen/verify 16-19 = 0x1010
8/27 - Handles multiple Gear box (non GB0) instances
8/30 - BER detects inverted pattern and Auto-verification
8/30 - Timer Id Change - Both BER and CAUIBER can run simultaneously
9/05 - Phase wheel is displayed correctly for both A0 and B0
9/06 - CDR Menu Panel uses updates startup_cdr.py
9/06 - CDR prbs25 enables Tx outputs 30.16-19[6]=0
9/06 - CAUI screens with not open if no GB present
9/06 - CAUI screens only show GB devices
9/06 - Added ipmdio_fdi_2c_B0 to gui directory. This adds to support with extra write for B0.
9/07 - Modified Detect Devices to recognize B0. It was also looking though 50 dev addresses instead of 32
9/10 - CAUI_RxEq-8888.py (CFP) script sets x_adjust offset for 4pt Eye
10/3 - Restore original CAUI Eye Colormap
10/3 - Individually name cauieye-Rx#.csv output file by lane (#=0-9)
10/3 - Fixed 4pt eye return zero after Full Eyescan
10/3 - AutoVerify also enables verification enable bit 12
10/3 - BER Tool displays Off if pattern verification disabled
10/17 - Disable FullSystem Read2Write after Run Script
10/18 - regWrite no longer calls regWriteAck
10/18 - Support GB0M instance call - Add ',device' to memWrite/Read Calls
10/18 - CAUIBER: All button only selects lanes that have CAUI input cdr lock 30.1546[9:0]
10/30 - ipmdio.py for sub-20 Add -ATE_log for mdio_wr only
11/1 - ipcauieye.py speed increase. use function DoVerticalStepsF
11/1 - ipcauimon2.py - add faster non-destructive 4pt eye measurement
11/6 - ipcauimon2.py - All locked button to select only lanes with cdr lock.
11/6 - ipeye.py Individual file naming
11/7 - ipeye.py Add legacy csv option
11/7 - ipeye.py Reimplement eye height

Requests:
CTLE when forced does not display fixed value.
 
Arg option to toggle between FDI/SUB-20
Arg option to reassign regWriteAck to regWrite