/*  gensec4 - A Traveller sector generator

    Based on Marc Miller, "Traveller Sector Generator", Using Your
	Model 2/BIS, Challenge No. 25; and other Traveller material
	(Referee's Manual and issues of Traveller's Digest.)

    The format of the output of gensec is a superset of the material
    produced by Mr. Miller's published program, and can be formatted
	in a number of ways.

	Each output file uses the first line to identify the version of the
	output format. This same string is used by the mapsub program to
	interpret the data correctly.

	The format line consists of the text "#Version: x.x" with the value
	of x.x depending on which style of output was chosen.

	Version numbers are as follows:

	Version	Description
	1.0	    Original Standard UPP .sec format
	2.0	    New Standard UWP format (GEnie) .sec format
	2.1		Heaven & Earth/Galactic .sec format
	2.2		Heaven & Earth .hes format
	2.3		Gensec/mapsec/subsec v2 .sec format
	2.5		travellermap.com API output .sec format
	3.0		TBD: Sector XML format

    This program rewritten C and Unix, Aug 18 1987, by James T. Perkins.
    (jamesp@dadla.la.tek.com, @uunet.uu.net:jamesp@dadla.la.tek.com)

    Additions made Mar 17 1989 by Fred Schiff
    (vu0141@bingvaxu.cc.binghamton.edu) to conform with Basic generation
    in MegaTraveller Referee's Manual.

	This program rewritten in C++, April 2009, by Chris Moynihan.
	(forthekill@gmail.com, forthekill.com)
	Additions were made to allow for multiple output formats, the
	ability to use a file with names and hexes to include pre-determined
	worlds was added, and to set density of 'zero'.
*/
/*
Copyright 1989 James T. Perkins

	This notice and any statement of authorship must be reproduced on all
	copies.  The author does not make any warranty expressed or implied,
	or assumes any liability or responsiblity for the use of this software.

	Any distributor of copies of this software shall grant the recipient
	permission for further redistribution as permitted by this notice. Any
	distributor must distribute this software without any fee or other
	monetary gains, unless expressed written permission is granted by the
	author.

	This software or its use shall not be: sold, rented, leased, traded, or
	otherwise marketed without the expressed written permission of the author.

	If the software is modified in a manner creating derivative	copyrights,
	appropriate legends may be placed on derivative work in addition to that
	set forth above.

	Permission is hereby granted to copy, reproduce, redistribute or
	otherwise use this software as long as the conditions above	are met.

	All rights not granted by this notice are reserved.
*/

/** HEADER INCLUDES **/
#include "anyoption.h"

/** SYSTEM INCLUDES **/
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdlib>
//#include <ctime>
using namespace std;

/** PREPROCESSOR DIRECTIVES **/
/* Maximum number of systems in a sector */
#define MAX_SYS 1280

/* Local macros */
#define D2 nDiceRoll(2, 6)
#define D1 diceRoll(6)
#define DM(test, dm) ((test) ? (dm) : 0)
#define limit(x, l, u) (((x) < (l)) ? (l) : (((x) > (u)) ? (u) : (x)))

/** STRUCTURE DEFINITIONS **/
/* For storing the options */
struct optionValues
{
	string subsecLetter;
	string density;
	string maturity;
	string allegience;
	string sectorName;
	string namesFilePath;
	int outputFormat;
	string outputPath;
};
/* For storing the location of systems read from the hex/names file */
struct starSystem
{
	int starHex;
	string starName;
	int xHex;
	int yHex;
};
/* For storing the generated systems */
struct generatedSystem
{
	string name;
	int hex;
	string UWP;
	char base;
	string codes;
	int PBG;
	string allegiance;
	char zone;
	string stellar;
	string satellite;
	string gasGiant;
};

/** STRUCTURE DECLARATIONS **/
/* Declare structure for command line options */
struct optionValues options;

/* Declare structure to store hex numbers and system names for pre-existing names file */
struct starSystem systemData[MAX_SYS];
struct starSystem *star_ptr = &systemData[0];

/* Declare structure for storing generated systems */
struct generatedSystem sys[MAX_SYS];
struct generatedSystem *genSys_ptr = &sys[0];

/** VARIABLE DECLARATIONS **/
/* Variables for controlling generation procedure */
int maturity = 3;	/* Determines how well travelled sector is */
int density = 50;	/* Stellar density for system presence */

string homePath = getenv("HOME");
string defaultSectorName = "Unnamed";   /* Name of the sector */
string defaultNamesFilePath = homePath + "/";      /* Path of hex/names file*/
string defaultOutputPath = homePath + "/";        /* Path of the output file*/
int defaultOutputFormat = 5;            /* Default output style */
string defaultAllegience = "Im";        /* Default allegience */

/* To keep count of where you are in the generatedSystems array */
int sdn = 1;

/* To count the number of generated systems */
int secDataLine = 1;


/** FORWARD DECLARATIONS **/
void getOptions( int argc, char* argv[] );
int readNamesFile();
void hexIterate(int fileExists);
void generateSystem(int x, int y, string ali, string hexName);
void writeSectorFile(int outFormat);
char hexChar(int i);
int diceRoll(int nsides);
int nDiceRoll(int ndice, int nsides);

/** MAIN PROGRAM **/
int
main( int argc, char* argv[] )
{
	srand((unsigned)time(NULL));

	getOptions( argc, argv );

	int fileExists = readNamesFile();

	if (fileExists == 0){
        hexIterate(fileExists);
	}else{
        hexIterate(fileExists);
	}

	writeSectorFile(options.outputFormat);

	return 0;
}

/* GETS THE COMMAND LINE ARGUMENTS */
void
getOptions( int argc, char* argv[] )
{

	/* 1. CREATE AN OBJECT */
	AnyOption *opt = new AnyOption();

	/* 2. SET PREFERENCES  */
	//opt->noPOSIX(); /* do not check for POSIX style character options */
	opt->setVerbose(); /* print warnings about unknown options */
	opt->autoUsagePrint(true); /* print usage for bad options */

	/* 3. SET THE USAGE/HELP   */
	opt->addUsage( "" );
	opt->addUsage( "Usage: " );
	opt->addUsage( "" );
	opt->addUsage( " -h  --help          Print usage " );
	opt->addUsage( " -L  --subsecLet     Letter of Subsector (A-P) to generate, if omitted will generate entire sector " );
	opt->addUsage( " -d  --density       %|zero|rift|sparse|scattered|dense " );
	opt->addUsage( " -m  --maturity      Tech level, backwater|frontier|mature|cluster " );
	opt->addUsage( " -a  --ac            Two-letter system alignment code " );
	opt->addUsage( " -s  --secName       Name of sector. For default output file name and sectorName_names.txt file" );
	opt->addUsage( " -p  --path          Path to sectorName_names.txt file " );
	opt->addUsage( " -o  --outFormat     1|2|3|4|5|6 : v1.0, v2.0, v2.1 v2.1b, v2.2, v2.5 " );
	opt->addUsage( " -u  --outPath       Path and name of output file " );
	opt->addUsage( "" );

	/* 4. SET THE OPTION STRINGS/CHARACTERS */
	opt->setCommandFlag( "help", 'h');
	opt->setCommandOption( "subsecLet", 'L');
	opt->setCommandOption( "density", 'd');
	opt->setCommandOption( "maturity", 'm');
	opt->setCommandOption( "ac", 'a');
	opt->setCommandOption( "secName", 's');
	opt->setCommandOption( "path", 'p');
	opt->setCommandOption( "outFormat", 'o');
	opt->setCommandOption( "outPath", 'u');

	/* 5. PROCESS THE COMMANDLINE AND RESOURCE FILE */
	/* go through the command line and get the options  */
	opt->processCommandArgs( argc, argv );

	if( ! opt->hasOptions()) { /* print usage if no options */
		opt->printUsage();
		delete opt;
		return;
	}

	/* 6. GET THE VALUES */
	if( opt->getFlag( "help" ) || opt->getFlag( 'h' ) )
		opt->printUsage();

	if( opt->getValue( 'L' ) != NULL  || opt->getValue( "subsecLet" ) != NULL  )
		options.subsecLetter = opt->getValue( 'L');

	if( opt->getValue( 'd' ) != NULL  || opt->getValue( "density" ) != NULL  )
		options.density = opt->getValue( 'd');

	if( opt->getValue( 'm' ) != NULL  || opt->getValue( "maturity" ) != NULL  )
		options.maturity = opt->getValue( 'm');

	if( opt->getValue( 'a' ) != NULL  || opt->getValue( "ac" ) != NULL  ){
		options.allegience = opt->getValue( 'a');
	}else{
	    options.allegience = defaultAllegience;
	}

	if( opt->getValue( 's' ) != NULL  || opt->getValue( "secName" ) != NULL  )
	{
	  	options.sectorName = opt->getValue( 's');
	}else{
	    options.sectorName = defaultSectorName;
	}

	if( opt->getValue( 'p' ) != NULL  || opt->getValue( "path" ) != NULL  )
	{
		options.namesFilePath = opt->getValue( 'p');
	}else{
	    options.namesFilePath = defaultNamesFilePath;
	}

	if( opt->getValue( 'o' ) != NULL  || opt->getValue( "outFormat" ) != NULL  ){
		options.outputFormat = atoi(opt->getValue( 'o'));
	}else{
	    options.outputFormat = defaultOutputFormat;
	}

    if( opt->getValue( 'u' ) != NULL  || opt->getValue( "outPath" ) != NULL  ){
        options.outputPath = opt->getValue( 'u');
    }else{
        if (options.outputFormat < 7){
            options.outputPath = defaultOutputPath + options.sectorName + ".sec";
            cout << "outputPath: " << options.outputPath << "\n";
        }else{
            options.outputPath = defaultOutputPath + options.sectorName + ".xml";
            cout << "outputPath: " << options.outputPath << "\n";
        }
    }

	/* Set Density */
	if (options.density.compare("dense") == 0){
		density = 66;
	}else if (options.density.compare("scattered") == 0){
		density = 33;
	}else if (options.density.compare("sparse") == 0){
		density = 16;
	}else if (options.density.compare("rift") == 0){
		density = 4;
	}else if (options.density.compare("zero") == 0){
		density = 0;
	}else{
		int densityInt = atoi(options.density.c_str());
		if ((densityInt >= 0) && (densityInt <= 100)){
			density = densityInt;
		}
	}

	/* Set Maturity */
	if (options.maturity.compare("backwater") == 0){
		maturity = 1;
	}else if (options.maturity.compare("frontier") == 0){
		maturity = 2;
	}else if (options.maturity.compare("mature") == 0){
		maturity = 3;
	}else if (options.maturity.compare("cluster") == 0){
		maturity = 4;
	}else{
		maturity = 3; /* Default is mature */
	}

	/* 8. DONE */
	delete opt;

}

/* READ THE NAMES/HEXES FOR PREDEFINED SYSTEMS, IF ANY */
int
readNamesFile()
{
	string line;
	stringstream fileName;

	fileName << options.namesFilePath << options.sectorName << "_names.txt";
	//cout << fileName.str().c_str() << "\n";

	ifstream inputFile;
	inputFile.open(fileName.str().c_str());

	if (!inputFile){
		return(0);
	}

	if (inputFile.is_open())
	{
		int count = 0;

		/* Read in the sectorname_names.txt file and populate the starSystem structure */
		while (getline (inputFile, line))
		{
			istringstream system(line);
			system >> systemData[count].starName >> systemData[count].starHex;

			/* Take the full hex number and break it into separate X and Y values*/
			systemData[count].xHex = systemData[count].starHex / 100;
			systemData[count].yHex = systemData[count].starHex % 100;
			count = count + 1;
		}
		inputFile.close();
		return(1);
	}
	return(1);
}

/* WALK THROUGH THE HEXES AND RANDOMLY CALL SYSTEM GENERATION */
void
hexIterate(int fileExists)
{
	int x, y;
	int x_start = 1, x_end = 32;
	int y_start = 1, y_end = 40;

	string hexName = "Unnamed";

	int lineNum = 0; /* Keep track of the line in the sectornames_names.txt file that we are on */

	/* Count through each hex and randomly (or not) generate a system */
	for (x = x_start; x <= x_end; x++)
	{
		for (y = y_start; y <= y_end; y++)
		{
			switch(fileExists){
			case 1:
				/* Check if the X value of the hex matches */
				if (systemData[lineNum].xHex == x)
				{
					/* Check if the Y value of the hex matches*/
					if (systemData[lineNum].yHex == y)
					{
						/* Grab the system name for the matched hex*/
						hexName.assign(systemData[lineNum].starName);
						/* Call system gen and pass the pre-defined system name */
						generateSystem (x, y, options.allegience, hexName);
						lineNum++;
						secDataLine++;
					}
					else
					{
						/* Y Hex didn't match, randomly generate a system */
						if (diceRoll(100) <= density)
						{
							hexName.assign("Unnamed");
							generateSystem (x, y, options.allegience, hexName);
							secDataLine++;
						}
					}
				}
				else
				{
					/* X Hex didn't match, randomly generate a system */
					if (diceRoll(100) <= density)
					{
					 	hexName.assign("Unnamed");
						generateSystem (x, y, options.allegience, hexName);
						secDataLine++;
					}
				}
				break;
			default:
                /* No names file, generate all systems randomly */
				if (diceRoll(100) <= density)
				{
					hexName.assign("Unnamed");
					generateSystem (x, y, options.allegience, hexName);
					secDataLine++;
				}
				break;
			}
		}
	}
	cout << "# of Systems: " << secDataLine << "\n";
}

/* GENERATE A SYSTEM */
void
generateSystem(int x, int y, string ali, string hexName)
{
	static int giants[] = {1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5};
    static int belts[] = {1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 3};
	string tra;
    char cla, bas, zon;
    int siz, atm, hyd, pop, gov, law, tl, gas, pla, mul;
    bool sco, nav, dep, mil, way;

    /* Starport class */
	int roll = D2 - 2;

	switch(maturity){
	case 1:
		cla = *("AABBCCCDEEX" + roll); /* backwater */
		break;
	case 2:
		cla = *("AAABBCCDEEX" + roll); /* frontier (standard) */
		break;
	case 3:
		cla = *("AAABBCCDEEE" + roll); /* mature */
		break;
	case 4:
		cla = *("AAAABBCCDEX" + roll); /* cluster */
		break;
	default:
		cla = *("AAABBCCDEEE" + roll); /* Default is mature */
		break;
	}

    /* Physical characteristics */
    siz = D2 - 2;
    atm = ((siz == 0) ? 0 : (D2 - 7 + siz));
    atm = limit(atm, 0, 15);
    hyd = D2 - 7 + siz + DM(atm < 2 || atm > 9, -4);
    hyd = ((siz < 2) ? 0 : hyd);
    hyd = limit(hyd, 0, 10);

    /* Demographics */
    pop = D2 - 2;
    gov = D2 - 7 + pop;
    gov = limit(gov, 0, 15);
    law = D2 - 7 + gov;
    law = limit(law, 0, 20);

    /* Technological Level */
    tl = D1 + DM(cla == 'A', 6) + DM(cla == 'B', 4) + DM(cla == 'C', 2) +
        DM(cla == 'X', -4);
    tl = tl + DM(siz < 5, 1) + DM(siz < 2, 1);
    tl = tl + DM(atm < 4, 1) + DM(atm > 9 && atm < 15, 1);
    tl = tl + DM(hyd == 8, 1) + DM(hyd == 9, 2);
    tl = tl + DM(pop > 0 && pop < 6, 1) + DM(pop == 9, 2) +
        DM(pop == 10, 4);
    tl = tl + DM(gov == 0 || gov == 5, 1) + DM(gov == 13, -2);
    tl = limit(tl, 0, 16);

    /* System characteristics (PBG) */
    mul = diceRoll(5) + ((D1 > 3) ? -1 : 4);/* population multiplier */
    pla = ((D2 < 8) ? 0 : belts[D2 - 2]);	/* planetoid belts */
    gas = ((D2 < 5) ? 0 : giants[D2 - 2]);	/* gas giants */

    /* Travel advisories */
	zon = ((cla == 'X') ? 'R' : ((D2 > 11) ? 'A' : ' '));

    /* Bases */
    nav = (cla < 'C' && D2 > 7);
    sco = (cla < 'E' && (D2 + DM(cla == 'A', -3) + DM(cla == 'B', -2) + DM(cla == 'C', -1)) > 6) ;
    mil = (cla < 'D' && (D2 + DM(pop > 8, -1) + DM((atm > 1 && atm < 6 && hyd < 4), -20)) > 11);
    dep = (cla < 'B' && gov > 9);
    way = (cla < 'B' && (hyd > 4));
    bas = (nav && sco ? 'A' : (nav && way ? 'B' : (way ? 'W' : (dep && nav ? 'D' : (nav ? 'N' : (sco ? 'S' : (mil ? 'M' : ' ')))))));

    /* Trade classifications */
    //tra = '\0';
    if (pop > 8)
        tra = tra + "Hi ";        /* High Population */
    if (pop < 4)
        tra = tra + "Lo ";        /* Low Population */
    if (pop == 0 && gov == 0 && law == 0)
        tra = tra + "Ba ";        /* Barren */
    if (atm > 3 && atm < 10 && hyd > 3 && hyd < 9 && pop > 4 && pop < 8)
        tra = tra + "Ag ";        /* Agricultural */
    if (atm < 4 && hyd < 4 && pop > 5)
        tra = tra + "Na ";        /* Non-Agricultural */
    if (((atm > 1 && atm < 5) || atm == 7 || atm == 9) && pop > 8)
        tra = tra + "In ";        /* Industrial */
    if (pop < 7)
        tra = tra + "Ni ";        /* Non-Industrial */
    if ((atm == 6 || atm == 8) && pop > 5 && pop < 9 && gov > 3 && gov < 10)
        tra = tra + "Ri ";        /* Rich */
    if (atm > 1 && atm < 6 && hyd < 4)
        tra = tra + "Po ";        /* Poor */
    if (hyd == 0 && atm > 1)
        tra = tra + "De ";        /* Desert World */
    if (hyd == 10)
        tra = tra + "Wa ";        /* Water World */
    if (siz == 0 && atm == 0 && hyd == 0)
        tra = tra + "As ";        /* Asteroid Belt */
    else if (atm == 0)
        tra = tra + "Va ";        /* Vaccuum World */
    if (siz > 9 && atm > 0)
        tra = tra + "Fl ";        /* Fluid */
    if (atm < 2 && hyd > 0)
        tra = tra + "Ic ";        /* Ice-Capped */

    /* Store the system */
	sys[sdn].name = hexName;
	sys[sdn].hex = (x*100) + y;

	sys[sdn].UWP = sys[sdn].UWP + cla;
	sys[sdn].UWP = sys[sdn].UWP + hexChar(siz);
	sys[sdn].UWP = sys[sdn].UWP + hexChar(atm);
	sys[sdn].UWP = sys[sdn].UWP + hexChar(hyd);
	sys[sdn].UWP = sys[sdn].UWP + hexChar(pop);
	sys[sdn].UWP = sys[sdn].UWP + hexChar(gov);
	sys[sdn].UWP = sys[sdn].UWP + hexChar(law);
	sys[sdn].UWP = sys[sdn].UWP + "-";
	sys[sdn].UWP = sys[sdn].UWP + hexChar(tl);

	sys[sdn].base = bas;
	sys[sdn].codes = tra;
	sys[sdn].PBG = (mul*100) + (pla*10) + gas;
	sys[sdn].allegiance = ali;
	sys[sdn].zone = zon;

	sys[sdn].stellar = "";
	sys[sdn].satellite = "";
	sys[sdn].gasGiant = "";

	sdn++;

}

/* WRITE THE SECTOR FILE */
void
writeSectorFile(int outFormat)
{
	/* This function writes all the sector data to the file format specified */
	int line = 1;
	stringstream outFileCat;

	/* Create output file */
	outFileCat << options.outputPath;
	string outFile = outFileCat.str();
	cout << "Output file: " << outFile << "\n";

	ofstream out(outFile.c_str(),ios::ate);

	switch(outFormat){
	case 1:
		/* .sec v1.0: Original Standard UPP Format */
		/* ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8 */
		/* 0101 FAFAAZS-L b Ag Hi In Ri Wa Im z g r r                                       */
		out << "#Version: 1.0\n";

		while(line < secDataLine){
			out << setw(4) << resetiosflags(ios::left) << setfill('0') << sys[line].hex << " ";
			out << setw(9) << setiosflags(ios::left) << sys[line].UWP << "  ";
			out << setw(1) << sys[line].base << " ";
			out << setw(14) << setfill(' ') << sys[line].codes << " ";
			out << setw(2) << sys[line].allegiance << " ";
			out << setw(1) << sys[line].zone << " ";
			out << setw(1) << resetiosflags(ios::left) << setfill('0') << sys[line].PBG % 1 << " ";

			if ((line + 1) < secDataLine){
				 out << "\n";
			}

			line++;
		}
		out.close();
		break;
	case 2:
		/* .sec v2.0: New Standard UWP Format (GEnie)  */
		/* ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8 */
		/* systemname123 0101 FAFAAZS-L  b Ag Hi In Ri Wa  z  pbg Im stellardata12345       */
		out << "#Version: 2.0\n";

		while(line < secDataLine){
			out << setw(13) << setiosflags(ios::left) << setfill(' ') << sys[line].name << " ";
			out << setw(4) << resetiosflags(ios::left) << setfill('0') << sys[line].hex << " ";
			out << setw(9) << setiosflags(ios::left) << sys[line].UWP << "  ";
			out << setw(1) << sys[line].base << " ";
			out << setw(14) << setfill(' ') << sys[line].codes << "  ";
			out << setw(1) << sys[line].zone << "  ";
			out << setw(3) << resetiosflags(ios::left) << setfill('0') << sys[line].PBG << " ";
			out << setw(2) << sys[line].allegiance;
			out << setw(16) << setiosflags(ios::left) << setfill(' ') << sys[line].stellar;

			if ((line + 1) < secDataLine){
				 out << "\n";
			}

			line++;
		}
		out.close();
		break;
	case 3:
		/* .sec v2.1: Heaven & Earth/Galactic .sec*/
		/* ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8 */
		/* systemnamehere0101 FAFAAZS-L  b Ag Hi In Ri Wa  z  pbg Im stellardata12345       */
		out << "#Version: 2.1\n";

		while(line < secDataLine){
			out << setw(14) << setiosflags(ios::left) << setfill(' ') << sys[line].name;
			out << setw(4) << resetiosflags(ios::left) << setfill('0') << sys[line].hex << " ";
			out << setw(9) << setiosflags(ios::left) << sys[line].UWP << "  ";
			out << setw(1) << sys[line].base << " ";
			out << setw(14) << setfill(' ') << sys[line].codes << "  ";
			out << setw(1) << sys[line].zone << "  ";
			out << setw(3) << resetiosflags(ios::left) << setfill('0') << sys[line].PBG << " ";
			out << setw(2) << sys[line].allegiance;
			out << setw(16) << setiosflags(ios::left) << setfill(' ') << sys[line].stellar;

			if ((line + 1) < secDataLine){
				 out << "\n";
			}

			line++;
		}
		out.close();
		break;
	case 4:
		/* .sec v2.2: Heaven & Earth .hes*/
		/* ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8----+ */
		/* 0101  systemnamehere  FAFAAZS-L  Ag Hi In Ri   pbg  b  Im  z  s  stellardatagoeshere1 */
		out << "#Version: 2.2\n";

		while(line < secDataLine){
			out << setw(4) << resetiosflags(ios::left) << setfill('0') << sys[line].hex << "  ";
			out << setw(14) << setiosflags(ios::left) << setfill(' ') << sys[line].name << "  ";
			out << setw(9) << setiosflags(ios::left) << sys[line].UWP << "  ";
			out << setw(12) << setfill(' ') << sys[line].codes << "  ";
			out << setw(3) << resetiosflags(ios::left) << setfill('0') << sys[line].PBG << "  ";
			out << setw(1) << sys[line].base << "  ";
			out << setw(2) << sys[line].allegiance << "  ";
			out << setw(1) << sys[line].zone << "     ";
			out << setw(20) << setiosflags(ios::left) << setfill(' ') << sys[line].stellar;

			if ((line + 1) < secDataLine){
				 out << "\n";
			}

			line++;
		}
		out.close();
		break;
	case 5:
		/* .sec v2.3: gensec/mapsub v2) */
		/* ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8 */
		/* systemnamegoeshere 0101 FAFAAZS-L b Ag Hi In Ri Wa  pbg Im z                     */
		out << "#Version: 2.3\n";

		while(line < secDataLine){
			out << setw(18) << setiosflags(ios::left) << setfill(' ') << sys[line].name << " ";
			out << setw(4) << resetiosflags(ios::left) << setfill('0') << sys[line].hex << " ";
			out << setw(9) << setiosflags(ios::left) << sys[line].UWP << " ";
			out << setw(1) << sys[line].base << " ";
			out << setw(15) << setfill(' ') << sys[line].codes << " ";
			out << setw(3) << resetiosflags(ios::left) << setfill('0') << sys[line].PBG << " ";
			out << setw(2) << sys[line].allegiance << " ";
			out << setw(1) << sys[line].zone;

			if ((line + 1) < secDataLine){
				 out << "\n";
			}

			line++;
		}
		out.close();
		break;
	case 6:
		/* .sec v2.5: travellermap.com */
		/* ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8 */
		/* systemnameis25characters1 0101 FAFAAZS-L b Ag Hi In Ri Wa            z pbg Im    */
		out << "#Version: 2.5\n";

		while(line < secDataLine){
			out << setw(25) << setiosflags(ios::left) << setfill(' ') << sys[line].name << " ";
			out << setw(4) << resetiosflags(ios::left) << setfill('0') << sys[line].hex << " ";
			out << setw(9) << setiosflags(ios::left) << sys[line].UWP << " ";
			out << setw(1) << sys[line].base << " ";
			out << setw(25) << setfill(' ') << sys[line].codes << " ";
			out << setw(1) << sys[line].zone << " ";
			out << setw(3) << resetiosflags(ios::left) << setfill('0') << sys[line].PBG << " ";
			out << setw(2) << sys[line].allegiance;

			if ((line + 1) < secDataLine){
				 out << "\n";
			}

			line++;
		}
		out.close();
		break;
	//case 7:
		/* .sec v3.0: Sector XML */
		/* Not Yet Implemented */
        //cout << "XML format not yet implemented" << "\n";
		//break;
	default:
		/* default is set to 6 */
        /* .sec v2.5: travellermap.com */
		/* ----+----1----+----2----+----3----+----4----+----5----+----6----+----7----+----8 */
		/* systemnameis25characters1 0101 FAFAAZS-L b Ag Hi In Ri Wa            z pbg Im    */
		out << "#Version: 2.5\n";

		while(line < secDataLine){
			out << setw(25) << setiosflags(ios::left) << setfill(' ') << sys[line].name << " ";
			out << setw(4) << resetiosflags(ios::left) << setfill('0') << sys[line].hex << " ";
			out << setw(9) << setiosflags(ios::left) << sys[line].UWP << " ";
			out << setw(1) << sys[line].base << " ";
			out << setw(25) << setfill(' ') << sys[line].codes << " ";
			out << setw(1) << sys[line].zone << " ";
			out << setw(3) << resetiosflags(ios::left) << setfill('0') << sys[line].PBG << " ";
			out << setw(2) << sys[line].allegiance;

			if ((line + 1) < secDataLine){
				 out << "\n";
			}

			line++;
		}
		out.close();
		break;
	}
}

/* CONVERT AN INT TO ITS HEX CHARACTER EQUIVALENT */
char
hexChar(int i)
{
    if (i < 0 || i > 34)
        return '?';
    else
        return *("0123456789ABCDEFGHJKLMNPQRSTUVWXYZ" + i);
}


/* ROLL A SINGLE DIE WITH n NUMBER OF SIDES */
int
diceRoll(int numSides)
{
	int rollResult;
	rollResult = rand() % numSides + 1;
    return (rollResult);
}


/* ROLL x NUMBER OF DICE WITH n NUMBER OF SIDES */
int
nDiceRoll(int numDice, int numSides)
{
    if (numDice < 1)
    {
        return 0;
    }
    else
    {
        return nDiceRoll(numDice - 1, numSides) + diceRoll(numSides);
    }
}
