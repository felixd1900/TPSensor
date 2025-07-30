XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXxxxxxxxxxxxxX
X   	Artwork and documentation done by: 			            X
X								            X
X	TEXAS INSTRUMENTS NORWAY LPRF              		            X
X  								            X
X	Address: Hoffsveien 70C    0377 OSLO	              	            X
X 	web: http://www.ti.com/lsds/ti/wireless-connectivity/overview.page  x
X                                                           	            X
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXxxxxxxxxxxxx

PCB NAME : CC1352REM-XD7793-XD24
REVISION: E0
DATE: 2018-06-12

FILE:  PACKED WITH WinZIP 

PCB DESCRIPTION:4 LAYER PCB 
Design Cross Section
TOP,CONDUCTOR,COPPER,0.035mm
DIELECTRIC,FR-4,0.175mm
L2,PLANE,COPPER,0.018mm
DIELECTRIC,FR-4,1.140000mm
L3,PLANE,COPPER,0.018mm
DIELECTRIC,FR-4,0.175mm
BOTTOM,CONDUCTOR,COPPER,0.035mm


Total Thickness: 1.60 MM (Tollerance +/- 0.16MM)


Via L1-L4

Soldermask color: GREEN

  IPC 4101/126 or equivalent substrate (Resin contents around 45%, which gives Er=4.4@2.4GHz, TanD=0.016)
  Dimensions in mm
  DOUBLE SIDE SOLDER MASK,
  DOUBLE SIDE SILKSCREEN,
  0.15 MM MIN TRACE WIDTH AND 0.15 MM MIN ISOLATION.

                 
FILE NAME            		DESCRIPTION                               	FILE TYPE
-------------------------------------------------------------------------------------------
***PCB MANUFACTURING FILES: (Folder Gerber)
Top.art                 	LAYER 1 TOP SIDE/POSITIV              		EXT. GERBER
Bottom.art                  	LAYER 4 SOLDER SIDE/POSITIV                 	EXT. GERBER
soldmask_top.art            	SOLDER MASK COMPONENT SIDE/NEGATIVE             EXT. GERBER
soldmask_bottom.art             SOLDER MASK SOLDER SIDE/NEGATIVE            	EXT. GERBER
silk_top.art	            	SILKSCREEN COMPONENT SIDE/POSITIVE              EXT. GERBER
silk_bottom.art		        SILKSCREEN SOLDER SIDE/POSITIVE             	EXT. GERBER
drill.art		        DRILL/MECHANICAL DRAWING                    	EXT. GERBER
art_param.txt                   Gerber report 				        ASCII
photoplot.log                   Gerber report					ASCII


***PCB DRILL FILES: (Folder Drill)
NCdrill-1-4.drl                 NC DRILL Plated		                       	EXCELLON
NCdrill-1-4-np.drl		NC DRILL non-Plated		 	        EXCELLON
ncdrill.log		        DRILL/NC DRILL REPORT                           ASCII
nc_pararm.txt		        PCB PHOTOPLOTTER DEFINITION FILE	        ASCII
nc_tools_auto.txt               Drill diameters				        ASCII


*** ASSEMBLY FILES: (Folder Assembly)
assembly_top.art	        ASSEMBLY DRAWING COMPONENT SIDE/NEGATIVE        EXT. GERBER
assembly_bot_mirror.art        	ASSEMBLY DRAWING SOLDER SIDE/NEGATIVE, MIRROR   EXT. GERBER
paste_top.art	             	SOLDER PAST COMPONENT SIDE/POSITIVE             EXT. GERBER
paste_bottom.art	        SOLDER PAST SOLDER SIDE/POSITIVE                EXT. GERBER
pick_and_place.txt            	PICK AND PLACE FILE 			        ASCII
CC1352REM-XD7793-XD24_partlist.xls 	BOM					Excel


*** Document files: (Folder docs)
CC1352REM-XD7793-XD24_schematic.pdf	Circuit Diagram				PDF
CC1352REM-XD7793-XD24_layout.pdf      	Layout Diagram				PDF
CC1352REM-XD7793-XD24_mechanical.pdf	Mechanical drawing			PDF
CC1352REM-XD7793-XD24_assembly.pdf	Assembly drawing        		PDF


README.TXT           			THIS FILE                     	        ASCII


Revision history:

R 1.0.0
--------------------------------------------------------------------------------------------
- Initial release revision. 

R 1.1.0
--------------------------------------------------------------------------------------------
- Added filter before balun on Sub1G path to redce radiated 3rd harmonic.

R 1.2.0
--------------------------------------------------------------------------------------------
- Added switch for common RF output
- Changed connection on EM pinheader P1 to get UART on P1.7 and P1.9

R 1.3.0
---------------------------------------------------------------------------------------------
- Updated Sub1G path with LC filter in front of balun to reduce radiated harmonics. 
- Reduced filter order after balun.

R E0
---------------------------------------------------------------------------------------------
- Update all part
- Change C231 from GRM155R61C105KA12D (end of life) to GRM155R61E105KA12D
- Adding option for external load capacitors on 48 MHz crystal.
- Changing revision from three digit to letter and digit.



END.
