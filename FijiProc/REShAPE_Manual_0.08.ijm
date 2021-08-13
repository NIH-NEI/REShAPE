//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// latest release date: 07/25/2016
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

/**
	All Epithelial Morphology Macros were developed by Nathan Hotaling

	Many of the images produced in this code were based off of those tools and principles developed in the BioVoxxel toolbox. 
	To give Jan Brocher credit for all the fantastic work, we include the following copyright notice and software disclaimer.

	Copyright (C) 2012-2016, Jan Brocher / BioVoxxel.

	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ?AS IS? AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED 
	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR 
	CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
	TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
	USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
	
	
*/

requires("1.50a");
IJorFIJI = getVersion();


//Define Crop size of all images to be analyzed and define which segmentation algorithms to use
		Dialog.create("Cell Morphological Assessment Tool");
		Dialog.setInsets(0, 150, 0);
			Dialog.addMessage("Graphic Options");
				Col_labels1 = newArray("All", "Just Neighbors", "All But Neighbors", "None");
					Dialog.addChoice("Create Colored Images:", Col_labels1, "All");
				
		Col_labels2 = newArray("Thermal", "Green", "mpl-magma","phase", "Fire", "Jet", "Cyan Hot");
				Dialog.addChoice("Coloring:", Col_labels2, "thermal");
				
		Col_labels3 = newArray("Both Tiff", "Tiff Stack+PNG Montage", "Tiff Stack","Tiff Montage", "PNG Montage", "Tiff Separate Images","PNG Separate Images");
			Dialog.addChoice("Image Format", Col_labels3, "Tiff Stack+PNG Montage");
				Dialog.addMessage("*Note: Image generation takes the majority of the processing time \n If Images are large Tif Stacks may fail");
				
		Dialog.setInsets(25, 103, 0);
		Dialog.addMessage("Cell Size Restrictions for Analysis");	
				Dialog.addString("Lower Cell Size (Pixels)", 50);
				Dialog.addString("Upper Cell Size (Pixels)", 12000);	
				
	Dialog.setInsets(25, 120, 0);
		Dialog.addMessage("Automated Unit Conversion");		
			radio_items = newArray("Yes", "No");
			Dialog.setInsets(0, 0, 0);
			Dialog.addRadioButtonGroup("Do you want to convert all output from pixels to real units?", radio_items, 1, 2, "Yes");
				Dialog.addNumber("Length of scale bar", 1, 0, 7, "Pixels");
				Dialog.addNumber("Length of scale bar", 1, 0 , 7, "Microns");	
		
		Dialog.show;
		
			graphic_choice = Dialog.getChoice();
			LUT_choice = Dialog.getChoice();
			graphic_format = Dialog.getChoice();	
			unit_conv = Dialog.getRadioButton();
			
			pore_lower = Dialog.getString();
			pore_upper = Dialog.getString();
			
			
			unit_pix = Dialog.getNumber();
			unit_real = Dialog.getNumber();

// Asks for a directory where Tif files are stored that you wish to analyze
	dir1 = getDirectory("Choose Source Directory ");
		list = getFileList(dir1);
		setBatchMode(true);
		
	T1 = getTime();	
	


	for (i=0; i<list.length; i++) {
		showProgress(i+1, list.length);
		filename = dir1 + list[i];
	if (endsWith(filename, "tif") || endsWith(filename, "tiff") ) {
		open(filename);
		

// Save Converted B&W image into a new folder called Processed
	myDir = dir1+"Segmented Images"+File.separator;

	File.makeDirectory(myDir);
		if (!File.exists(myDir))
			exit("Unable to create directory");
		
// Save Analysis of Particles to a csv file in this directory
	myDir1 = dir1+"Analysis"+File.separator;

	File.makeDirectory(myDir1);
		if (!File.exists(myDir1))
			exit("Unable to create directory");

if(list.length>1){
// Saves Numbered Binary Outlines to Subdirectory labeled Counted Borders
	myDir2 = dir1+"Combined Files"+File.separator;

	File.makeDirectory(myDir2);
		if (!File.exists(myDir2))
			exit("Unable to create directory");		
	}
// Saves Colored Images to Subdirectory labeled Counted Borders
	myDir3 = dir1+"Color Coded"+File.separator;

	File.makeDirectory(myDir3);
		if (!File.exists(myDir3))
			exit("Unable to create directory");					
			
// Sets Scale of picture to pixels 
	run("Set Scale...", "distance=0  known=0 pixel=1 unit= pixels");
	
// Creates custom file names for use later
		var name0=getTitle;
			name0a = replace(name0,"_outlines.tif","");
		var name1=name0+"_Counted";
			name1= replace(name1,".tif","");
		var name2=name0+"_Data";
			name2= replace(name2,".tif","");
		var name3=name0+"_Outlines";
			name3= replace(name3,".tif","");
		name4 = name0 + "_Neighbors";
			name4= replace(name4,".tif","");
		name5= name0 + "_Neighbors";
			name5= replace(name5,".tif","");
		name6= name0 + "_Neighbor Legend";
			name6= replace(name6,".tif","");
		name7= name0 + "_Visualized";
			name7= replace(name7,".tif","");
		name8= name0 + "_Metrics Legend";
			name8= replace(name8,".tif","");	
		name9= name0 + "_Montage";
			name9= replace(name9,".tif","");				
		
// Creates custom file paths for use later
		path0 = myDir+name0;
		path1 = myDir3+name1;
		path2 = myDir1+name2;
		path3 = myDir+name3;
		path4 = myDir3+name4;
		path5 = myDir1+name5;
		path6 = myDir3+name6;
		path7 = myDir3+name7;
		path8 = myDir3+name8;
		path9 = myDir3+name9;
		
if(isOpen("Log")==1) { selectWindow("Log"); run("Close"); }		

//Creates some variables for use in the code below.		
		lower = 0;
		upper = 0;	
		size = pore_lower + "-" + pore_upper;
//		size = "0-Infinity";
		wid=getWidth();
		hig=getHeight();
		vscale= 30;
		wscale= 5;
		totalw= floor(wid/wscale);
		totalh= floor(hig/vscale);
		totalht= totalh*1.5;
		color= "black" ;
		method= "Particle Neighborhood";
		hoodRadius= 5;
		watershed= false;
		circularity= "0.00-1.00";
		excludeEdges= true;
		calibrationbar= true;
		createPlot= true;
		

// Saves B&W cell outlines for analysis		
	run("Select None");
		run("Invert");
			run("Voronoi");	
		//run("Threshold...");
			setThreshold(lower+1, upper+255);
				setOption("BlackBackground", true);
				run("Convert to Mask");
				run("Make Binary");
					run("8-bit");
			saveAs("Tiff", path3);
		close();

				
//*****************************************************************************************************************************************************************************************************************		
// Create a neighbor analysis and color map 
				
	
//Opens files of interest and parses them into new variable values	
	open(path3+".tif");
		
		name0 = getTitle;
			name0= replace(name0,"_Data.tif","");
		
		original=getTitle();
		type=is("binary");
		if(type==false) { exit("works only with 8-bit binary images"); }
			getDimensions(width, height, channels, slices, frames);
			run("Options...", "iterations=1 count=1 black edm=Overwrite do=Nothing");
				if(isOpen("Log")==1) { selectWindow("Log"); print("\\Clear"); }
	print("Finding Neighbors for " + name0);
	
//Setup
	selectWindow(original);
		run("Select None");
	
		run("Invert");
		edges="exclude";
	
//prepare original image for analysis
	if(unit_conv == "Yes") {
		run("Set Scale...", "distance=unit_pix known=unit_real unit=microns");
		}
	
		run("Analyze Particles...", "size="+size+" pixel circularity="+circularity+" show=Masks "+edges+" clear");
		run("Invert LUT");
		rename(original+"-1");
		original=getTitle();
	
	run("Duplicate...", "title=[NbHood_"+original+"]");
		neighborhood=getTitle();
		selectWindow(neighborhood);
		run("Set Measurements...", "  centroid redirect=None decimal=3");
	if(unit_conv == "Yes") {
		run("Set Scale...", "distance=unit_pix known=unit_real unit=microns");
		}
		run("Analyze Particles...", "size="+size+" pixel circularity=0.00-1.00 show=Nothing clear record");
	
		
//define variables
		initialParticles=nResults;
		X=newArray(nResults);
		Y=newArray(nResults);
		neighborArray=newArray(nResults);
		neighbors=0;
		mostNeighbors=0;
		
		
//retveive Cell coordinates
	run("Set Scale...", "distance=0  known=0 pixel=1 unit= pixels");
		for(l=0; l<initialParticles; l++) {
			X[l]=getResult("XStart", l);
			Y[l]=getResult("YStart", l);
			toUnscaled(X[l], Y[l]);
		}
		
		
//prepare selector image
		setForegroundColor(255, 255, 255);
		setBackgroundColor(0, 0, 0);
		run("Set Measurements...", " centroid redirect=None decimal=3");
		run("Wand Tool...", "mode=Legacy tolerance=0");
		run("Options...", "iterations=1 count=1 black edm=Overwrite do=Nothing");
			
		
//create selector neighborhood
		for(hood=0; hood<initialParticles; hood++) {
			selectWindow(neighborhood);
				run("Select None");
				run("Duplicate...", "title=[Selector_"+original+"]");
					selector=getTitle();
					doWand(X[hood], Y[hood]);
			run("Enlarge...", "enlarge="+hoodRadius);
				run("Fill");
				run("Select None");
					doWand(X[hood], Y[hood]);
					selectWindow(neighborhood);
				run("Restore Selection");
			run("Analyze Particles...", "size="+size+" pixel circularity=0.00-1.00 show=Nothing clear record");
				neighbors = nResults-1;
				neighborArray[hood]=neighbors;
			
			if(neighbors>mostNeighbors) {
				mostNeighbors=neighbors;	
			}
			close(selector);
			
		}

	//if(mostNeighbors==0) {
	//	exit("no neighbors detected\ndid you choose the correct particle color?");
	//}
	
	Neighbor_metric  = newArray(nResults);
	Neighbor_metric  = neighborArray;

	
//Color coded original features
	
if(graphic_choice == "All" || graphic_choice == "Just Neighbors") {	
	selectWindow(original);
		run("Duplicate...", "title=[P-NbHood_"+hoodRadius+"_"+original+"]");
	particles=getTitle();

	selectWindow(particles);
	
	for(mark=0; mark<initialParticles; mark++) {
		markValue=neighborArray[mark];
		setForegroundColor(markValue, markValue, markValue);
		floodFill(X[mark],Y[mark], "8-connected");
	}
	
	run("Select None");		
	run("glasbey");
	close(original);
	}
	
//create distribution plot and neighbor files
		neighborList = newArray(mostNeighbors+1);
		Array.fill(neighborList, 0);
		for(num=0; num<initialParticles; num++) {
			nextNeighbor = neighborArray[num];
			if(nextNeighbor>0) {
				neighborList[nextNeighbor] = neighborList[nextNeighbor] + 1;
				}
		}

//		Plot.create("Distribution: " + particles, "neighbors", "count", neighborList);
//		Plot.addText("particles (total) = " + initialParticles, 0.01, 0.1);
//		setBatchMode("show");
		
		l3 = neighborList.length;
		Neighcat = Array.getSequence(l3);
		NeighCount = neighborList;
			run("Clear Results");
			
			for (ki=0; ki<l3; ki++) {
				setResult("Neighbors", ki, Neighcat[ki]);
				setResult(name0a, ki, NeighCount[ki]);
			}
			
			saveAs("Results", path5+".csv");
			run("Clear Results");
			
if(graphic_choice == "All" || graphic_choice == "Just Neighbors") {		
// Calibration Bar

// Define dimensions of the calibration bar
		stepsize=-floor(-totalw/mostNeighbors);
			totalw_new = stepsize*mostNeighbors;
		newImage("Calibration_"+original, "8-bit Black", totalw_new, totalh, 1);

// Fill each step with a new color		
			step=0;
		for(c=0; c<=mostNeighbors; c++) {
				makeRectangle(step, 0, stepsize, totalh);
				setForegroundColor(c+1, c+1, c+1);
					run("Fill");
					step=step+stepsize;
			}
		run("Select None");
		run("glasbey");
		run("RGB Color");
		setForegroundColor(255, 255, 255);
		setBackgroundColor(0, 0, 0);
		run("Canvas Size...", "width=totalw_new height=totalht position=Top-Center");
				setJustification("left");
				setFont("SansSerif", 0.7573*totalh*0.5-0.1783, "Bold");
			setJustification("center");
				for(il=1; il<=mostNeighbors; il++) {	
					drawString(il, stepsize*(il-1)+stepsize/2, totalht-1);
				}

		run("Select All");
			w2 = getWidth();
			h2 = getHeight();
			run("Copy");
			setPasteMode("Copy");
				selectWindow(particles);
					run("RGB Color");
					run("Select None");
					makeRectangle(1, 1, w2, h2); 
				run("Paste");	
				run("Select None");
				saveAs("Tiff",path4);
				run("Close All");		
		}
	
//*********************************************************************************************************************************************************************************************


//Create Color Maps for All Cell Shape Descriptors
	open(path3+".tif");
	setBatchMode(true);
	getDimensions(width, height, channels, slices, frames);
		run("Options...", "iterations=1 count=1 black edm=Overwrite do=Nothing");
		
		original=getTitle();
		color = "black";
		watershed = false;
		excludeEdges= true;
		includeHoles = false;
		calibrationbar = true;
		distributionPlot = true;
		LUT = LUT_choice;
		total_pixels = getWidth()*getHeight();
		edges = "exclude";
		holes = "";	


			selectWindow(original);
			
			if(graphic_choice == "All" || graphic_choice == "Just Neighbors") {				
				run("Select None");
					run("Invert");
				}
			if(graphic_choice == "None" || graphic_choice == "All But Neighbors") {				
				run("Select None");
			}					
				
	if(unit_conv == "Yes") {
		run("Set Scale...", "distance=unit_pix known=unit_real unit=microns");
		}
	run("Set Measurements...", "area mean standard modal min centroid center perimeter bounding fit shape feret's integrated median skewness kurtosis area_fraction redirect = None decimal = 3");

	run("Analyze Particles...", "size=size pixel circularity=0.00-1.00 show = Masks "+edges+" clear "+holes+" record");



		run("Invert LUT");
			rename("Input");
			input = getTitle();
				run("Duplicate...", "title = ShapeDescr_"+original);
					result = getTitle();
		
				allParticles = nResults;
					X = newArray(allParticles);
					Y = newArray(allParticles);
					Area = newArray(allParticles);
					Peri = newArray(allParticles);
					AoP = newArray(allParticles);
					ElipMaj = newArray(allParticles);
					ElipMin = newArray(allParticles);
					AR = newArray(allParticles);
					Angle = newArray(allParticles);
					Feret = newArray(allParticles);
					MinFeret = newArray(allParticles);
					FeretAR = newArray(allParticles);
					FeretAngle = newArray(allParticles);
					Circ = newArray(allParticles);
					Solidity = newArray(allParticles);
					Compactness = newArray(allParticles);
					Extent = newArray(allParticles);
					PoN = newArray(allParticles);
					Poly_SR = newArray(allParticles);
					Poly_AR = newArray(allParticles);
					Hex_SR = newArray(allParticles);
					Hex_AR = newArray(allParticles);
					Poly_Ave = newArray(allParticles);
					Hex_Ave = newArray(allParticles);
					A_hull = newArray(allParticles);
					P_hull = newArray(allParticles);
				

//read in positional information
	for(ni = 0; ni<allParticles; ni++) {
	
		X[ni] = getResult("X", ni);
		Y[ni] = getResult("Y", ni);
		toUnscaled(X[ni], Y[ni]); 

//read in shape descriptors 
		Area[ni] = getResult("Area", ni);
		Peri[ni] = getResult("Perim.", ni);
		AoP[ni] = (getResult("Area", ni)/getResult("Perim.", ni));
			setResult("Area/Perim.", ni, AoP[ni]);
		ElipMaj[ni] = getResult("Major", ni);
		ElipMin[ni] = getResult("Minor", ni);
		AR[ni] = getResult("AR", ni);
		Angle[ni] = getResult("Angle", ni);
		Feret[ni] = getResult("Feret", ni);
		MinFeret[ni] = getResult("MinFeret", ni);
		FeretAR[ni] = (getResult("Feret", ni)/getResult("MinFeret", ni));
			setResult("Ferets AR", ni, FeretAR[ni]);
		FeretAngle[ni] = getResult("FeretAngle", ni);
		Circ[ni] = getResult("Circ.", ni);
		Solidity[ni] = getResult("Solidity", ni);
		Compactness[ni] = (sqrt((4/PI)*getResult("Area", ni))/getResult("Major", ni));
			setResult("Compactness", ni, Compactness[ni]);
		Extent[ni] = (getResult("Area", ni)/((getResult("Width", ni))*(getResult("Height", ni))));
			setResult("Extent", ni, Extent[ni]);
		setResult("Neighbors", ni, Neighbor_metric[ni]);
		A_hull[ni] = getResult("Area",ni)/getResult("Solidity", ni);
			setResult("Area Convext Hull", ni, A_hull[ni]);
		P_hull[ni] = 6*sqrt(A_hull[ni]/(1.5*sqrt(3)));
			setResult("Perim Convext Hull", ni, P_hull[ni]);	
			
// Perimeter divided by area metric		
		if(Neighbor_metric[ni] == 0){
			PoN[ni] = NaN;
				setResult("PoN", ni, PoN[ni]);
		}
		if(Neighbor_metric[ni] > 0){
			PoN[ni] = (getResult("Perim.",ni)/getResult("Neighbors",ni));
				setResult("PoN", ni, PoN[ni]);
		}

// Polygonality metrics calculated based on the number of sides of the polygon 		
		if(Neighbor_metric[ni] > 2){		
			Poly_SR[ni] = 1-sqrt((1-(PoN[ni]/(sqrt((4*Area[ni])/(Neighbor_metric[ni]*(1/(tan(PI/Neighbor_metric[ni]))))))))*(1-(PoN[ni]/(sqrt((4*Area[ni])/(Neighbor_metric[ni]*(1/(tan(PI/Neighbor_metric[ni])))))))));
				setResult("PSR", ni, Poly_SR[ni]);
			Poly_AR[ni] = 1-sqrt((1-(Area[ni]/(0.25*Neighbor_metric[ni]*PoN[ni]*PoN[ni]*(1/(tan(PI/Neighbor_metric[ni]))))))*(1-(Area[ni]/(0.25*Neighbor_metric[ni]*PoN[ni]*PoN[ni]*(1/(tan(PI/Neighbor_metric[ni])))))));
				setResult("PAR", ni, Poly_AR[ni]);
			Poly_Ave[ni] = 10*(Poly_SR[ni]+Poly_AR[ni])/2;
				setResult("Poly_Ave", ni, Poly_Ave[ni]);	

//Hexagonality metrics calculated based on a convex, regular, hexagon 				
				apoth1 = sqrt(3)*Peri[ni]/12;
				apoth2 = sqrt(3)*Feret[ni]/4;
				apoth3 = MinFeret[ni]/2;
				
				side1 = Peri[ni]/6;
				side2 = Feret[ni]/2;
				side3 = MinFeret[ni]/sqrt(3);
				side4 = P_hull[ni]/6;	

// Unique area calculations from the derived and primary measures above    
				Area1 = 0.5*(3*sqrt(3))*side1*side1;
				Area2 = 0.5*(3*sqrt(3))*side2*side2;
				Area3 = 0.5*(3*sqrt(3))*side3*side3;
				Area4 = 3*side1*apoth2;
				Area5 = 3*side1*apoth3;
				Area6 = 3*side2*apoth3;
				Area7 = 3*side4*apoth1;
				Area8 = 3*side4*apoth2;
				Area9 = 3*side4*apoth3;
				Area10 = A_hull[ni];
				Area11 = Area[ni];
// Create an array of all unique areas				
					Area_uniq = newArray(Area1,Area2,Area3,Area4,Area5,Area6,Area7,Area8,Area9,Area10,Area11);

// Create an array of the ratio of all areas to eachother					
					Area_ratio = newArray(55);
					jv = 0;
					//run("Set Measurements...", " nan redirect=None decimal=6");

					for(ib=0; ib< Area_uniq.length; ib++){
						for(ic=ib+1; ic<Area_uniq.length; ic++){
							Area_ratio[jv] = 1-sqrt((1-(Area_uniq[ib]/Area_uniq[ic]))*(1-(Area_uniq[ib]/Area_uniq[ic])));
								jv = jv+1;
						}
					}

// Create Summary statistics of all array ratios					
					Array.getStatistics(Area_ratio, min, max, mean, stdDev);
					  Area_Ratio_Min = min;
					  Area_Ratio_Max = max;
					  Area_Ratio_Ave = mean;
					  Area_Ratio_SD = stdDev;
					  
//Set the hexagon area ratio equal to the average Area Ratio			  
			Hex_AR[ni] = Area_Ratio_Ave;
				setResult("HAR", ni, Hex_AR[ni]);
				

// Perimeter Ratio Calculations
// Two extra apothems are now useful                 
				apoth4 = sqrt(3)*P_hull[ni]/12;
				apoth5 = sqrt(4*A_hull[ni]/(4.5*sqrt(3)));
				
				Perim1 = sqrt(24*Area[ni]/sqrt(3));
				Perim2 = sqrt(24*A_hull[ni]/sqrt(3));
				Perim3 = Peri[ni];
				Perim4 = P_hull[ni];
				Perim5 = 3*Feret[ni];
				Perim6 = 6*MinFeret[ni]/sqrt(3);
				Perim7 = 2*Area[ni]/(apoth1);
				Perim8 = 2*Area[ni]/(apoth2);
				Perim9 = 2*Area[ni]/(apoth3);
				Perim10 = 2*Area[ni]/(apoth4);
				Perim11 = 2*Area[ni]/(apoth5);
				Perim12 = 2*A_hull[ni]/(apoth1);
				Perim13 = 2*A_hull[ni]/(apoth2);
				Perim14 = 2*A_hull[ni]/(apoth3);

					Perim_uniq = newArray(Perim1,Perim2,Perim3,Perim4,Perim5,Perim6,Perim7,Perim8,Perim9,Perim10,Perim11,Perim12,Perim13,Perim14);

// Create an array of the ratio of all Perims to eachother					
					Perim_ratio = newArray(91);
					jv = 0;
					//run("Set Measurements...", " nan redirect=None decimal=6");
					for(ib=0; ib< Perim_uniq.length; ib++){
						for(ic=ib+1; ic<Perim_uniq.length; ic++){
							Perim_ratio[jv] = 1-sqrt((1-(Perim_uniq[ib]/Perim_uniq[ic]))*(1-(Perim_uniq[ib]/Perim_uniq[ic])));
								jv = jv+1;
						}
					}
// Create Summary statistics of all array ratios					
					Array.getStatistics(Perim_ratio, min, max, mean, stdDev);
					  Perim_Ratio_Min = min;
					  Perim_Ratio_Max = max;
					  Perim_Ratio_Ave = mean;
					  Perim_Ratio_SD = stdDev;
			Hex_SR[ni] = Perim_Ratio_Ave;
				setResult("HSR", ni, Hex_SR[ni]);
				
			Hex_Ave[ni] = 10*(Hex_AR[ni]+Hex_SR[ni])/2;	
				setResult("Hex_Ave", ni, Hex_Ave[ni]);
		}
		

		if(Neighbor_metric[ni] < 3){
			Poly_SR[ni] = NaN;
				setResult("PSR", ni, Poly_SR[ni]);
			Poly_AR[ni] = NaN;
				setResult("PAR", ni, Poly_AR[ni]);
			Poly_Ave[ni] = NaN;
				setResult("Poly_Ave", ni, Poly_Ave[ni]);
			Hex_SR[ni] = NaN;
				setResult("HSR", ni, Hex_SR[ni]);
			Hex_AR[ni] = NaN;
				setResult("HAR", ni, Hex_AR[ni]);
			Hex_Ave[ni] = NaN;
				setResult("Hex_Ave", ni, Hex_Ave[ni]);				
		}
		print(nResults);
	}
			saveAs("Results", path2+".csv");
	
if(graphic_choice == "All" || graphic_choice == "All But Neighbors") {	
		run("Summarize");
		
					biggestArea = getResult("Area", nResults-1);
					biggestPeri = getResult("Perim.", nResults-1);
					biggestAoP = getResult("Area/Perim.", nResults-1);
					biggestElipMaj = getResult("Major", nResults-1);
					biggestElipMin = getResult("Minor", nResults-1);
					biggestAR = getResult("AR", nResults-1);
					biggestAngle = getResult("Angle", nResults-1);
					biggestFeret = getResult("Feret", nResults-1);
					biggestMinFeret = getResult("MinFeret", nResults-1);
					biggestFeretAR = getResult("Ferets AR", nResults-1);
					biggestFeretAngle = getResult("FeretAngle", nResults-1);
					biggestCirc = getResult("Circ.", nResults-1);
					biggestSolidity = getResult("Solidity", nResults-1);
					biggestCompactness = getResult("Compactness", nResults-1);
					biggestExtent = getResult("Extent", nResults-1);
					biggestPoN = getResult("PoN", nResults-1);
					biggestNei = getResult("Neighbors", nResults-1);
					biggestPSR = getResult("PSR", nResults-1);
					biggestPAR = getResult("PAR", nResults-1);
					biggestPA = getResult("Poly_Ave", nResults-1);
					biggestHSR = getResult("HSR", nResults-1);					
					biggestHAR = getResult("HAR", nResults-1);
					biggestHA = getResult("Hex_Ave", nResults-1);
					
					
					smallestArea = getResult("Area", nResults-2);
					smallestPeri = getResult("Perim.", nResults-2);
					smallestAoP = getResult("Area/Perim.", nResults-2);
					smallestElipMaj = getResult("Major", nResults-2);
					smallestElipMin = getResult("Minor", nResults-2);
					smallestAR = getResult("AR", nResults-2);
					smallestAngle = getResult("Angle", nResults-2);
					smallestFeret = getResult("Feret", nResults-2);
					smallestMinFeret = getResult("MinFeret", nResults-2);
					smallestFeretAR = getResult("Ferets AR", nResults-2);
					smallestFeretAngle = getResult("FeretAngle", nResults-2);
					smallestCirc = getResult("Circ.", nResults-2);
					smallestSolidity = getResult("Solidity", nResults-2);
					smallestCompactness = getResult("Compactness", nResults-2);
					smallestExtent = getResult("Extent", nResults-2);
					smallestPoN = getResult("PoN", nResults-2);
					smallestNei = getResult("Neighbors", nResults-2);
					smallestPSR = getResult("PSR", nResults-2);
					smallestHSR = getResult("HSR", nResults-2);
					smallestPAR = getResult("PAR", nResults-2);
					smallestHAR = getResult("HAR", nResults-2);
					smallestPA = getResult("Poly_Ave", nResults-2);
					smallestHA = getResult("Hex_Ave", nResults-2);					
					
	selectWindow("Results");
		run("Close");
	
		biggest_Name = newArray("Area", "Perimeter", "Area/Perimeter","Perimeter/Neighbors",
			"Ellipse Major Axis","Ellipse Minor Axis","Ellipse Aspect Ratio", "Ellipse Major Axis Angle",
			"Max. Feret Diameter","Min. Feret Diameter","Feret Aspect Ratio",  "Max Feret Diam. Angle", 
			"Circularity", "biggestSolidity", "Compactness", "Extent", 
			"Polygon Side Ratio","Polygon Area Ratio", "Polygonality Score",
			"Hexagon Side Ratio", "Hexagon Area Ratio","Hexagonality Score");
			
		biggestValue = newArray(biggestArea,biggestPeri, biggestAoP, biggestPoN,
			biggestElipMaj,biggestElipMin,biggestAR,biggestAngle,
			biggestFeret,biggestMinFeret,biggestFeretAR,biggestFeretAngle,
			biggestCirc,biggestSolidity,biggestCompactness,biggestExtent,
			biggestPSR,biggestPAR,biggestPA,
			biggestHSR,biggestHAR,biggestHA);
			
		smallestValue = newArray(smallestArea,smallestPeri, smallestAoP, smallestPoN,
			smallestElipMaj,smallestElipMin,smallestAR,smallestAngle,
			smallestFeret,smallestMinFeret,smallestFeretAR,smallestFeretAngle,
			smallestCirc,smallestSolidity,smallestCompactness,smallestExtent,
			smallestPSR,smallestPAR,smallestPA,
			smallestHSR,smallestHAR,smallestHA);
	
	bigValue = newArray(biggestValue.length);
	midValue = newArray(biggestValue.length);
	smallValue = newArray(biggestValue.length);
	
	for(ie=0; ie<biggestValue.length; ie++){
		midValue[ie] = smallestValue[ie]+((biggestValue[ie]-smallestValue[ie])/2);
		bigValue[ie] = d2s(biggestValue[ie],2);
		midValue[ie] = d2s(midValue[ie],2);
		smallValue[ie] = d2s(smallestValue[ie],2);
	}

//	biggest_Array = Array.show("Biggest Metrics",biggest_Name,biggestValue);
	

	
//*******************************************************************

//color code shape descriptor maps
	shapeDescriptors=newArray("Area", "Perim.", "AoP", "PoN", 
							"Major", "Minor", "AR", "Angle", 
							"Feret", "MinFeret", "FeretAR", "FeretAngle", 
							"Circ.", "Solidity", "Compactness", "Extent",
							"PSR", "PAR", "Poly_Ave", "HSR", "HAR", "Hex_Ave");
	
	mapNames = newArray("Segmentation","Counted Cells", "Neighbors", "Area", "Perimeter", "Area/Perimeter", "Perimeter/Neighbors", 
						"Ellipse Major Axis", "Ellipse Minor Axis", "Ellipse Aspect Ratio", "Ellipse Angle", 
						"Feret Major", "Feret Minor", "Feret Aspect Ratio", "Feret Angle", 
						"Circularity", "Solidity", "Compactness", "Extent",
						"Polygon Side Ratio", "Polygon Area Ratio", "Polygonality Score",
						"Hexagon Side Ratio",  "Hexagon Area Ratio", "Hexagonality Score");
						
	n1 = shapeDescriptors.length;
	n2 = n1-1;
	n3 = mapNames.length;
	
//Creates an image stack that is as long as the metrics it wants to measure
	setPasteMode("Copy");
	selectWindow(result);
		setBatchMode("hide");
		for(nS = 1; nS<n3; nS++) {
			run("Add Slice"); 
		}
		run("Select None");
		
//Opens vornoi of cells
	selectWindow(input);
		run("Close");
			open(path3+".tif");
			input = getTitle();
				run("8-bit");
				
	
		run("Misc...", "divide=Infinity hide run");


// For each metric highlights the cell at the centroid position and then fills it with a greyscale value, then copy and pastes that filled 
// image into the stack created above.
	for(m=0; m<n1; m++) {
		selectWindow(input);
		run("Duplicate...", "title=["+mapNames[m]+"]");
		map=getTitle();
		selectWindow(map);
			HSR_Vals = newArray(allParticles);
		for(yi=0; yi<allParticles; yi++) {
			doWand(X[yi],Y[yi]);
			if(shapeDescriptors[m]=="Area") {
				value=5+round(250/(biggestArea-smallestArea)*(Area[yi]-smallestArea));
			}
			if(shapeDescriptors[m]=="Perim.") {
				value=5+round(250/(biggestPeri-smallestPeri)*(Peri[yi]-smallestPeri));
			}
			if(shapeDescriptors[m]=="AoP") {
				value=5+round(250/(biggestAoP-smallestAoP)*(AoP[yi]-smallestAoP));
			}
			if(shapeDescriptors[m]=="PoN") {
				if(PoN[yi] >= 0){
					value=5+round(250/(biggestPoN-smallestPoN)*(PoN[yi]-smallestPoN));
				}
				if(isNaN(PoN[yi])){
					value = 0;
				}
			}
			if(shapeDescriptors[m]=="Major") {
				value=5+round(250/(biggestElipMaj-smallestElipMaj)*(ElipMaj[yi]-smallestElipMaj));
			}
			if(shapeDescriptors[m]=="Minor") {
				value=5+round(250/(biggestElipMin-smallestElipMin)*(ElipMin[yi]-smallestElipMin));
			}				
			if(shapeDescriptors[m]=="AR") {
				value=5+round(250/(biggestAR-smallestAR)*(AR[yi]-smallestAR));
			}		
			if(shapeDescriptors[m]=="Angle") {
				value=5+round(250/(biggestAngle-smallestAngle)*(Angle[yi]-smallestAngle));
			}			
			if(shapeDescriptors[m]=="Feret") {
				value=5+round(250/(biggestFeret-smallestFeret)*(Feret[yi]-smallestFeret));
			}
			if(shapeDescriptors[m]=="MinFeret") {
				value=5+round(250/(biggestMinFeret-smallestMinFeret)*(MinFeret[yi]-smallestMinFeret));
			}
			if(shapeDescriptors[m]=="FeretAR") {
				value=5+round(250/(biggestFeretAR-smallestFeretAR)*(FeretAR[yi]-smallestFeretAR));
			}			
			if(shapeDescriptors[m]=="FeretAngle") {
				value=5+round(250/(biggestFeretAngle-smallestFeretAngle)*(FeretAngle[yi]-smallestFeretAngle));
			}
			if(shapeDescriptors[m]=="Circ.") {
				value=5+round(250/(biggestCirc-smallestCirc)*(Circ[yi]-smallestCirc));
			}
			if(shapeDescriptors[m]=="Solidity") {
				value=5+round(250/(biggestSolidity-smallestSolidity)*(Solidity[yi]-smallestSolidity));
			}
			if(shapeDescriptors[m]=="Compactness") {
				value=5+round(250/(biggestCompactness-smallestCompactness)*(Compactness[yi]-smallestCompactness));
			}
			if(shapeDescriptors[m]=="Extent") {
				value=5+round(250/(biggestExtent-smallestExtent)*(Extent[yi]-smallestExtent));
			}
			if(shapeDescriptors[m]=="PSR") {
				if(Poly_SR[yi]*Poly_SR[yi] >= 0){
					value=5+round(250/(biggestPSR-smallestPSR)*(Poly_SR[yi]-smallestPSR));
				}
				if(isNaN(Poly_SR[yi])){
					value = 0;
				}
			}
			if(shapeDescriptors[m]=="PAR") {
				if(Poly_AR[yi]*Poly_AR[yi] >= 0){
					value=5+round(250/(biggestPAR-smallestPAR)*(Poly_AR[yi]-smallestPAR));
				}
				if(isNaN(Poly_AR[yi])){
					value = 0;
				}
			}
			if(shapeDescriptors[m]=="Poly_Ave") {
				if(Poly_Ave[yi]*Poly_Ave[yi] >= 0){
					value=5+round(250/(biggestPA-smallestPA)*(Poly_Ave[yi]-smallestPA));
				}
				if(isNaN(Poly_Ave[yi])){
					value = 0;
				}
			}			
			if(shapeDescriptors[m]=="HSR") {
				if(Hex_SR[yi]*Hex_SR[yi] >= 0){
					value=5+round(250/(biggestHSR-smallestHSR)*(Hex_SR[yi]-smallestHSR));
				}
				if(isNaN(Hex_SR[yi])){
					value = 0;
				}
			}
			
			if(shapeDescriptors[m]=="HAR") {
				if(Hex_AR[yi]*Hex_AR[yi] >= 0){
					value=5+round(250/(biggestHAR-smallestHAR)*(Hex_AR[yi]-smallestHAR));
				}
				if(isNaN(Hex_AR[yi])){
					value = 0;
				}
			}			
			if(shapeDescriptors[m]=="Hex_Ave") {
				if(Hex_Ave[yi]*Hex_Ave[yi] >= 0){
					value=5+round(250/(biggestHA-smallestHA)*(Hex_Ave[yi]-smallestHA));
				}
				if(isNaN(Hex_Ave[yi])){
					value = 0;
				}
			}	
		
			setForegroundColor(value, value, value);
			run("Fill");
			setBatchMode("hide");
			
		}
		showProgress(((m+1)*yi)/(allParticles*10));
			run("Select All");
				run("Copy");
			selectWindow(result);
				run("Select None");
					run("16-bit");
				setSlice(m+4);
					run("Paste");
					run("8-bit");
				run("Select None");
		close(map);
	}

	
//Applies IDs to each window of what each metric is	
	selectWindow(result);
		currentID = getImageID();
		setSlice(1);
			run("Select All");
			run("Copy");
	setPasteMode("Transparent-white");
	selectWindow(result);
		for(mask=1; mask<=n3; mask++) {
			setSlice(mask);
				run("Paste");
					if(mask>=0) {
						setMetadata("Label", mapNames[mask-1]);
					}
		}
		close(input);
	
		setSlice(1);
		run("Select None");	
			run(LUT);
			run("Select None");	
				run("RGB Color");
			
//Create a calibration bar and paste it onto each image after labeling
		for(mask=4; mask<=n3; mask++) {
			setSlice(mask);		
				wid=getWidth();
				hig=getHeight();
				vscale= 30;
				wscale= 5;
				totalw= floor(wid/wscale);
				totalh= floor(hig/vscale);
				totalht= totalh*1.5;

			newImage("Calibration Bar", "8-bit Ramp", totalw, totalh, 1);
				
					step=0;
				run("Select None");
				run(LUT);
					run("RGB Color");
						setForegroundColor(255, 255, 255);
						setBackgroundColor(0, 0, 0);
			run("Canvas Size...", "width=totalw height=totalht position=Top-Center");
				setFont("SansSerif", 0.7573*totalh*0.5-0.1783, "Bold");
				
				setJustification("left");
			drawString(smallValue[mask-4], 2, totalht-1);
			
				setJustification("center");
			drawString(midValue[mask-4], totalw/2, totalht-1);
			
				setJustification("right");
			drawString(bigValue[mask-4], totalw, totalht-1);
			
			run("Select All");
				w3 = getWidth();
				h3 = getHeight();
				run("Copy");
					setPasteMode("Copy");
				selectWindow(result);
			makeRectangle(0, 0, w3, h3); 
			run("Paste");
					}			
	
	
// Creates an image of all cell outlines that is numbered
		open(path3+".tif");
		run("Invert");
			imgh = getHeight();
			fimgh = 0.7573*imgh*0.04-0.1783;
				if(fimgh > 25){
					fimgh =25;
				}
			
		run("Set Measurements...", "  redirect=None decimal=3");
				call("ij.plugin.filter.ParticleAnalyzer.setFontSize", fimgh); 
			run("Analyze Particles...", "size="+size+" pixelf show=Outlines exclude clear");
			
				run("Select All");
					run("RGB Color");
					run("Invert");
				run("Copy");
				setPasteMode("Copy");
			selectWindow(result);	
				setSlice(2);
				run("Paste");

		if(graphic_choice == "All" || graphic_choice == "Just Neighbors") {			
			open(path4+".tif");
				run("Select All");
				run("Copy");
				setPasteMode("Copy");
			selectWindow(result);	
				setSlice(3);
				run("Paste");
			}	
			x = nSlices;
			
			cls = -floor(-sqrt(x));
			rws = -floor(-(x/cls));
			slic1 = 1;
			
			scl1 = cls*getWidth();
			max_width =  900*cls;
			
			if(scl1 > max_width){
				scl_x = max_width/(cls*getWidth());
				mntg_font = 0.7573*max_width*0.025-0.1783;
			} else {
					scl_x = 1;
					mntg_font = 0.7573*getWidth()*0.1-0.1783;
				}
				run("Select None");
				
			open(path3+".tif");
				run("Select All");
				run("Copy");
				setPasteMode("Copy");
			selectWindow(result);	
				setSlice(1);
				run("Paste");
				run("Select None");

// Last Two if statements to incorporate are if graphic_format = "Tiff Separate Images" or "PNG Separate Images"
	
					if(graphic_format == "Both Tiff" || graphic_format == "Tiff Stack+PNG Montage" || graphic_format == "Tiff Stack"){
						saveAs("Tiff",path7);
					}

					if(graphic_format == "Both Tiff" || graphic_format == "Tiff Montage"){
					setFont("SansSerif", mntg_font, "Bold");
						run("Make Montage...", "columns=cls rows=rws scale=scl_x first=slic1  border=1 font=mntg_font label");
							saveAs("Tiff", path9);	
					}
					
					if(graphic_format == "Tiff Stack+PNG Montage" || graphic_format == "PNG Montage"){
					setFont("SansSerif", mntg_font, "Bold");
						run("Make Montage...", "columns=cls rows=rws scale=scl_x first=slic1  border=1 font=mntg_font label");
							saveAs("PNG", path9);	
					}	
				
						run("Clear Results");
					File.delete(path4+".tif");
					File.delete(path1+".tif");
			run("Close All");
	}
	}
print("\\Clear");


	}
	
	
	
	

//********************************************************************************************************************************************************************************************************************************	

// Analysis for combining Cell Morphological Features
		list2 = getFileList(myDir1);
			setBatchMode(true);
			
		log_morph = newArray(list2.length);
		c = 0;
		for (ij=0; ij<list2.length; ij++) {
			filename1 = list2[ij];
			if (endsWith(filename1, "_Data.csv")) {
				c = c+1;
				log_morph[c-1] = filename1;
				}
		}
		
		c= newArray(c);		
		l1 = c.length;
		
	for(ij =0; ij<l1; ij++){ 
		filedir2 = myDir1 + log_morph[ij];
		filename2 = log_morph[ij];
			

//Opens files of interest and parses them into new variable values	
			open(filedir2);
				n = nResults;
				poreX = newArray(n);
				poreY = newArray(n);
				
				poreval = newArray(n);
				poreperim = newArray(n);
				poreneig = newArray(n);
				poreAoP = newArray(n);
				porePoN = newArray(n);
				
				porePSR = newArray(n);
				poreHSR = newArray(n);
				porePAR = newArray(n);
				poreHAR = newArray(n);
				porePS = newArray(n);
				poreHS = newArray(n);
				
				poremaj = newArray(n);
				poremin = newArray(n);
				poreAR = newArray(n);
				poreangle = newArray(n);
				
				poreferet = newArray(n);
				poreminfer = newArray(n);
				poreferAR = newArray(n);				
				poreferetA = newArray(n);
				
				porewidth = newArray(n);
				poreheight = newArray(n);
				
				porecirc = newArray(n);
				poresolid = newArray(n);
				porecompact = newArray(n);
				poreextent = newArray(n);
				
				porenamenew = newArray(n);
				
				
				
				for (ik=0; ik<n; ik++) {
					poreX[ik] = getResult("X", ik);
					poreY[ik] = getResult("Y", ik);
					
					poreval[ik] = getResult("Area", ik);
					poreperim[ik] = getResult("Perim.", ik);
					poreneig[ik] = getResult("Neighbors", ik);
					poreAoP[ik] = getResult("Area/Perim.", ik);
					porePoN[ik] = getResult("PoN", ik);
					
					porePSR[ik] = getResult("PSR", ik);
					porePAR[ik] = getResult("PAR", ik);
					porePS[ik] = getResult("Poly_Ave", ik);
					poreHSR[ik] = getResult("HSR", ik);
					poreHAR[ik] = getResult("HAR", ik);					
					poreHS[ik] = getResult("Hex_Ave", ik);
			
					poremaj[ik] = getResult("Major", ik);
					poremin[ik] = getResult("Minor", ik);
					poreAR[ik] = getResult("AR", ik);
					poreangle[ik] = getResult("Angle", ik);
					
					poreferet[ik] = getResult("Feret", ik);
					poreminfer[ik] = getResult("MinFeret", ik);
					poreferAR[ik] = getResult("Ferets AR", ik);
					poreferetA[ik] = getResult("FeretAngle", ik);
					
					porewidth[ik] = getResult("Width", ik);
					poreheight[ik] = getResult("Height", ik);
					
					porecirc[ik] = getResult("Circ.", ik);
					poresolid[ik] = getResult("Solidity", ik);
					porecompact[ik] = getResult("Compactness", ik);
					poreextent[ik] = getResult("Extent", ik);
					
					porenamenew[ik] = filename2; 
				}
				run("Clear Results");

// Sets up initial results storage file for each set of values of interest			
		if(ij==0){
			for (il=0; il<n; il++) {
				setResult("Cell Centroid X Coord. (Pixel)", il, poreX[il]);
				setResult("Cell Centroid Y Coord. (Pixel)", il, poreY[il]);
				
				setResult("Cell Area", il, poreval[il]);
				setResult("Cell Perimeter", il, poreperim[il]);
				setResult("Cell Neighbors", il, poreneig[il]);
				setResult("Area/Perimeter", il, poreAoP[il]);
				setResult("Perimeter/Neighbors", il, porePoN[il]);
				
				setResult("PSR", il, porePSR[il]);
				setResult("PAR", il, porePAR[il]);
				setResult("Polygonality Score", il, porePS[il]);
				setResult("HSR", il, poreHSR[il]);
				setResult("HAR", il, poreHAR[il]);
				setResult("Hexagonality Score", il, poreHS[il]);
				
				setResult("Major Cell Axis", il, poremaj[il]);
				setResult("Minor Cell Axis", il, poremin[il]);
				setResult("Aspect Ratio", il, poreAR[il]);
				setResult("Angle of Major Axis", il, poreangle[il]);
				
				setResult("Ferets Max Diameter", il, poreferet[il]);
				setResult("Ferets Min Diameter", il, poreminfer[il]);
				setResult("Ferets Aspect Ratio", il, poreferAR[il]);
				setResult("Angle of F.Max Diam.", il, poreferetA[il]);
				
				setResult("Cell Bounding Box Width", il, porewidth[il]);
				setResult("Cell Bounding Box Height", il, poreheight[il]);
				
				setResult("Cell Circularity", il, porecirc[il]);
				setResult("Cell Solidity", il, poresolid[il]);
				setResult("Cell Compactness", il, porecompact[il]);
				setResult("Cell Extent", il, poreextent[il]);
				
				setResult("File Name", il, porenamenew[il]);
			} 
		
			selectWindow("Results");
				IJ.renameResults("temp.csv");
				print("\\Clear");
		}

// Adds current values of interest to saved storage file		
		if(ij>0 && ij < l1-1){
			selectWindow("Results");
				run("Close");
	
			selectWindow("temp.csv");
				IJ.renameResults("Results");
				old_len = nResults;
				old_poreX = newArray(old_len);
				old_poreY = newArray(old_len);
				
				old_porearea = newArray(old_len);
				old_poreperim = newArray(old_len);
				old_poreneig = newArray(old_len);
				old_poreAoP = newArray(old_len);
				old_porePoN = newArray(old_len);
				
				old_porePSR = newArray(old_len);
				old_porePAR = newArray(old_len);
				old_porePS = newArray(old_len);
				old_poreHSR = newArray(old_len);
				old_poreHAR = newArray(old_len);
				old_poreHS = newArray(old_len);
				
				old_poremaj = newArray(old_len);
				old_poremin = newArray(old_len);
				old_poreAR = newArray(old_len);
				old_poreangle = newArray(old_len);
				
				old_poreferet = newArray(old_len);
				old_poreminfer = newArray(old_len);
				old_poreferAR = newArray(old_len);
				old_poreferetA = newArray(old_len);
				
				old_porewidth = newArray(old_len);
				old_poreheight = newArray(old_len);

				old_porecirc = newArray(old_len);
				old_poresolid = newArray(old_len);
				old_porecomp = newArray(old_len);
				old_poreextent = newArray(old_len);
				
				oldporename = newArray(old_len);

			for (im=0; im<old_len; im++) {
				old_poreX[im] = getResult("Cell Centroid X Coord. (Pixel)", im);
				old_poreY[im] = getResult("Cell Centroid Y Coord. (Pixel)", im);
				
				old_porearea[im] = getResult("Cell Area", im);
				old_poreperim[im] = getResult("Cell Perimeter", im);
				old_poreneig[im] = getResult("Cell Neighbors", im);
				old_poreAoP[im] = getResult("Area/Perimeter", im);
				old_porePoN[im] = getResult("Perimeter/Neighbors", im);

				old_porePSR[im] = getResult("PSR", im);
				old_porePAR[im] = getResult("PAR", im);
				old_porePS[im] = getResult("Polygonality Score", im);
				old_poreHSR[im] = getResult("HSR", im);
				old_poreHAR[im] = getResult("HAR", im);
				old_poreHS[im] = getResult("Hexagonality Score", im);
				
				old_poremaj[im] = getResult("Major Cell Axis", im);
				old_poremin[im] = getResult("Minor Cell Axis", im);
				old_poreAR[im] = getResult("Aspect Ratio", im);
				old_poreangle[im] = getResult("Angle of Major Axis", im);
				
				old_poreferet[im] = getResult("Ferets Max Diameter", im);
				old_poreminfer[im] = getResult("Ferets Min Diameter", im);
				old_poreferAR[im] = getResult("Ferets Aspect Ratio", im);
				old_poreferetA[im] = getResult("Angle of F.Max Diam.", im);
				
				old_porewidth[im] = getResult("Cell Bounding Box Width", im);
				old_poreheight[im] = getResult("Cell Bounding Box Height", im);

				old_porecirc[im] = getResult("Cell Circularity", im);
				old_poresolid[im] = getResult("Cell Solidity", im);
				old_porecomp[im] = getResult("Cell Compactness", im);
				old_poreextent[im] = getResult("Cell Extent", im);	
				
				oldporename[im] = getResultString("File Name", im);			
			}
			
//Concatenates old data and new data together
		newpore_X = Array.concat(old_poreX, poreX);
		newpore_Y = Array.concat(old_poreY, poreY);

		newpore_area = Array.concat(old_porearea, poreval);
		newpore_perim = Array.concat(old_poreperim, poreperim);
		newpore_neig = Array.concat(old_poreneig, poreneig);
		newpore_AoP = Array.concat(old_poreAoP,poreAoP);
		newpore_PoN = Array.concat(old_porePoN,porePoN);
		
		newpore_PSR = Array.concat(old_porePSR, porePSR);
		newpore_PAR = Array.concat(old_porePAR, porePAR);
		newpore_PS = Array.concat(old_porePS,porePS);
		newpore_HSR = Array.concat(old_poreHSR, poreHSR);
		newpore_HAR = Array.concat(old_poreHAR, poreHAR);
		newpore_HS = Array.concat(old_poreHS,poreHS);
		
		newpore_maj = Array.concat(old_poremaj, poremaj);
		newpore_min = Array.concat(old_poremin, poremin);
		newpore_AR = Array.concat(old_poreAR, poreAR);
		newpore_angle = Array.concat(old_poreangle, poreangle);
		
		newpore_feret = Array.concat(old_poreferet, poreferet);
		newpore_minfer = Array.concat(old_poreminfer, poreminfer);
		newpore_ferAR = Array.concat(old_poreferAR, poreferAR);
		newpore_feretA = Array.concat(old_poreferetA, poreferetA);
		
		newpore_width = Array.concat(old_porewidth, porewidth);
		newpore_height = Array.concat(old_poreheight, poreheight);

		newpore_circ = Array.concat(old_porecirc, porecirc);
		newpore_solid = Array.concat(old_poresolid, poresolid);
		newpore_comp = Array.concat(old_porecomp, porecompact);
		newpore_extent = Array.concat(old_poreextent, poreextent);
		
		newporename = Array.concat(oldporename, porenamenew);


			n4 = newpore_area.length;
			run("Clear Results");
			
			for (il=0; il<n4; il++) {
				setResult("Cell Centroid X Coord. (Pixel)", il, newpore_X[il]);
				setResult("Cell Centroid Y Coord. (Pixel)", il, newpore_Y[il]);
				
				setResult("Cell Area", il, newpore_area[il]);
				setResult("Cell Perimeter", il, newpore_perim[il]);
				setResult("Cell Neighbors", il, newpore_neig[il]);
				setResult("Area/Perimeter", il, newpore_AoP[il]);
				setResult("Perimeter/Neighbors", il, newpore_PoN[il]);
				
				setResult("PSR", il, newpore_PSR[il]);
				setResult("PAR", il, newpore_PAR[il]);
				setResult("Polygonality Score", il, newpore_PS[il]);
				setResult("HSR", il, newpore_HSR[il]);
				setResult("HAR", il, newpore_HAR[il]);
				setResult("Hexagonality Score", il, newpore_HS[il]);
				
				setResult("Major Cell Axis", il, newpore_maj[il]);
				setResult("Minor Cell Axis", il, newpore_min[il]);
				setResult("Aspect Ratio", il, newpore_AR[il]);
				setResult("Angle of Major Axis", il, newpore_angle[il]);
				
				setResult("Ferets Max Diameter", il, newpore_feret[il]);
				setResult("Ferets Min Diameter", il, newpore_minfer[il]);
				setResult("Ferets Aspect Ratio", il, newpore_ferAR[il]);
				setResult("Angle of F.Max Diam.", il, newpore_feretA[il]);
				
				setResult("Cell Bounding Box Width", il, newpore_width[il]);
				setResult("Cell Bounding Box Height", il, newpore_height[il]);
				
				setResult("Cell Circularity", il, newpore_circ[il]);
				setResult("Cell Solidity", il, newpore_solid[il]);
				setResult("Cell Compactness", il, newpore_comp[il]);
				setResult("Cell Extent", il, newpore_extent[il]);
				
				setResult("File Name", il, newporename[il]);
			} 
			
				selectWindow("Results");
					IJ.renameResults("temp.csv");
				print("\\Clear");
		}

// Adds current values of interest to saved storage file		
		if(ij == l1-1){
			selectWindow("Results");
				run("Close");
		
			selectWindow("temp.csv");
				IJ.renameResults("Results");
				old_len = nResults;
				old_poreX = newArray(old_len);
				old_poreY = newArray(old_len);
				
				old_porearea = newArray(old_len);
				old_poreperim = newArray(old_len);
				old_poreneig = newArray(old_len);
				old_poreAoP = newArray(old_len);
				old_porePoN = newArray(old_len);
				
				old_porePSR = newArray(old_len);
				old_porePAR = newArray(old_len);
				old_porePS = newArray(old_len);
				old_poreHSR = newArray(old_len);
				old_poreHAR = newArray(old_len);
				old_poreHS = newArray(old_len);
				
				old_poremaj = newArray(old_len);
				old_poremin = newArray(old_len);
				old_poreAR = newArray(old_len);
				old_poreangle = newArray(old_len);
				
				old_poreferet = newArray(old_len);
				old_poreminfer = newArray(old_len);
				old_poreferAR = newArray(old_len);
				old_poreferetA = newArray(old_len);
				
				old_porewidth = newArray(old_len);
				old_poreheight = newArray(old_len);

				old_porecirc = newArray(old_len);
				old_poresolid = newArray(old_len);
				old_porecomp = newArray(old_len);
				old_poreextent = newArray(old_len);
				
				oldporename = newArray(old_len);


			for (im=0; im<old_len; im++) {
				old_poreX[im] = getResult("Cell Centroid X Coord. (Pixel)", im);
				old_poreY[im] = getResult("Cell Centroid Y Coord. (Pixel)", im);
				
				old_porearea[im] = getResult("Cell Area", im);
				old_poreperim[im] = getResult("Cell Perimeter", im);
				old_poreneig[im] = getResult("Cell Neighbors", im);
				old_poreAoP[im] = getResult("Area/Perimeter", im);
				old_porePoN[im] = getResult("Perimeter/Neighbors", im);
				
				old_porePSR[im] = getResult("PSR", im);
				old_porePAR[im] = getResult("PAR", im);
				old_porePS[im] = getResult("Polygonality Score", im);
				old_poreHSR[im] = getResult("HSR", im);
				old_poreHAR[im] = getResult("HAR", im);
				old_poreHS[im] = getResult("Hexagonality Score", im);
				
				old_poremaj[im] = getResult("Major Cell Axis", im);
				old_poremin[im] = getResult("Minor Cell Axis", im);
				old_poreAR[im] = getResult("Aspect Ratio", im);
				old_poreangle[im] = getResult("Angle of Major Axis", im);
				
				old_poreferet[im] = getResult("Ferets Max Diameter", im);
				old_poreminfer[im] = getResult("Ferets Min Diameter", im);
				old_poreferAR[im] = getResult("Ferets Aspect Ratio", im);
				old_poreferetA[im] = getResult("Angle of F.Max Diam.", im);
				
				old_porewidth[im] = getResult("Cell Bounding Box Width", im);
				old_poreheight[im] = getResult("Cell Bounding Box Height", im);

				old_porecirc[im] = getResult("Cell Circularity", im);
				old_poresolid[im] = getResult("Cell Solidity", im);
				old_porecomp[im] = getResult("Cell Compactness", im);
				old_poreextent[im] = getResult("Cell Extent", im);	
				
				oldporename[im] = getResultString("File Name", im);			
			}
			
		newpore_X = Array.concat(old_poreX, poreX);
		newpore_Y = Array.concat(old_poreY, poreY);

		newpore_area = Array.concat(old_porearea, poreval);
		newpore_perim = Array.concat(old_poreperim, poreperim);
		newpore_neig = Array.concat(old_poreneig, poreneig);
		newpore_AoP = Array.concat(old_poreAoP,poreAoP);
		newpore_PoN = Array.concat(old_porePoN,porePoN);
		
		newpore_PSR = Array.concat(old_porePSR, porePSR);
		newpore_PAR = Array.concat(old_porePAR, porePAR);
		newpore_PS = Array.concat(old_porePS,porePS);
		newpore_HSR = Array.concat(old_poreHSR, poreHSR);
		newpore_HAR = Array.concat(old_poreHAR, poreHAR);
		newpore_HS = Array.concat(old_poreHS,poreHS);	
		
		newpore_maj = Array.concat(old_poremaj, poremaj);
		newpore_min = Array.concat(old_poremin, poremin);
		newpore_AR = Array.concat(old_poreAR, poreAR);
		newpore_angle = Array.concat(old_poreangle, poreangle);
		
		newpore_feret = Array.concat(old_poreferet, poreferet);
		newpore_minfer = Array.concat(old_poreminfer, poreminfer);
		newpore_ferAR = Array.concat(old_poreferAR, poreferAR);
		newpore_feretA = Array.concat(old_poreferetA, poreferetA);
		
		newpore_width = Array.concat(old_porewidth, porewidth);
		newpore_height = Array.concat(old_poreheight, poreheight);

		newpore_circ = Array.concat(old_porecirc, porecirc);
		newpore_solid = Array.concat(old_poresolid, poresolid);
		newpore_comp = Array.concat(old_porecomp, porecompact);
		newpore_extent = Array.concat(old_poreextent, poreextent);
		
		newporename = Array.concat(oldporename, porenamenew);
			
			n= newpore_area.length;
			run("Clear Results");
			
			
			for (il=0; il<n; il++) {	
				setResult("Cell_Centroid_XCoord", il, newpore_X[il]);
				setResult("Cell_Centroid_YCoord", il, newpore_Y[il]);
				
				setResult("Cell_Area", il, newpore_area[il]);
				setResult("Cell_Perimeter", il, newpore_perim[il]);
				setResult("Cell_Neighbors", il, newpore_neig[il]);
				setResult("Area_Perimeter", il, newpore_AoP[il]);
				setResult("Perimeter_Neighbors", il, newpore_PoN[il]);
				
				setResult("Polygonality_Side_Ratio", il, newpore_PSR[il]);
				setResult("Polygonality_Area_Ratio", il, newpore_PAR[il]);
				setResult("Polygonality_Score", il, newpore_PS[il]);
				setResult("Hexagonality_Side_Ratio", il, newpore_HSR[il]);
				setResult("Hexagonality_Area_Ratio", il, newpore_HAR[il]);
				setResult("Hexagonality_Score", il, newpore_HS[il]);
				
				setResult("Major_Cell_Axis", il, newpore_maj[il]);
				setResult("Minor_Cell_Axis", il, newpore_min[il]);
				setResult("Aspect_Ratio", il, newpore_AR[il]);
				setResult("Angle_Major_Axis", il, newpore_angle[il]);
				
				setResult("Ferets_Max_Diameter", il, newpore_feret[il]);
				setResult("Ferets_Min_Diameter", il, newpore_minfer[il]);
				setResult("Ferets_Aspect_Ratio", il, newpore_ferAR[il]);
				setResult("Angle_Feret_Max", il, newpore_feretA[il]);
				
				setResult("Cell_Bounding_Box_Width", il, newpore_width[il]);
				setResult("Cell_Bounding_Box_Height", il, newpore_height[il]);
				
				setResult("Cell_Circularity", il, newpore_circ[il]);
				setResult("Cell_Solidity", il, newpore_solid[il]);
				setResult("Cell_Compactness", il, newpore_comp[il]);
				setResult("Cell_Extent", il, newpore_extent[il]);
				
				setResult("File_Name", il, newporename[il]);
			} 
				
				selectWindow("Results");
				run("Summarize");
					saveAs("Results", myDir2+"All Cell Metric Values.csv");
						run("Close");
				print("\\Clear");
			}
			
		}
		
		
			
//**********************************************************************************************************************************************************************************************
// Analysis for combining Neighbor Counts
		list3 = getFileList(myDir1);
			setBatchMode(true);
			
		log_neig = newArray(list3.length);
		c = 0;
		for (ij=0; ij<list3.length; ij++) {
			filename1 = list3[ij];
			if (endsWith(filename1, "_Neighbors.csv")) {
				c = c+1;
				log_neig[c-1] = filename1;
				}
		}
		
		c= newArray(c);		
		l2 = c.length;
		newname = newArray(l2);
		
	if(l2 > 1) { 
//Creates a list of each of the files of interest
		for(ij =0; ij<l2; ij++){ 
			filedir3 = myDir1 + log_neig[ij];
			filename4 = log_neig[ij];
				newname[ij] =  replace(filename4,"_Neighbors.csv","")+".tif";
				

//Opens files of interest and parses them into new variable values	
			open(filedir3);
			
				n = nResults;
				neighbor_val = newArray(n);
				neighbor_count = newArray(n);
					name99 = replace(filename4,"_Neighbors.csv","");
					name99 = name99+".tif";
				
				for (ik=0; ik<n; ik++) {
					neighbor_val[ik] = getResult("Neighbors", ik);
					neighbor_count[ik] = getResult(name99, ik);
				}
				
				run("Clear Results");
				
		if(ij==0){
			for (il=0; il<n; il++) {
				setResult("Neighbors", il, neighbor_val[il]);
				setResult(name99, il, neighbor_count[il]);
			} 
		
			selectWindow("Results");
				IJ.renameResults("temp.csv");
				print("\\Clear");
			}
			
// Adds current values of interest to saved storage file		
		if(ij>0 && ij < l2-1){
			selectWindow("Results");
				run("Close");

			selectWindow("temp.csv");
				IJ.renameResults("Results");
			
			for (im=0; im<n; im++) {
				setResult(name99, im, neighbor_count[im]);
			}
				selectWindow("Results");
					IJ.renameResults("temp.csv");
		}
		
// Adds current values of interest to saved storage file		
		if(ij == l2-1){
			selectWindow("Results");
				run("Close");
				
			selectWindow("temp.csv");
				IJ.renameResults("Results");	
				
			for (im=0; im<n; im++) {
				setResult(name99, im, neighbor_count[im]);
			}
				selectWindow("Results");
					IJ.renameResults("temp.csv");
				print("\\Clear");
			}

		}
		
			selectWindow("temp.csv");
				IJ.renameResults("Results");	
			final_n = nResults;
			
			for (j=0; j<final_n; j++) {
				c = 0; 
					for(i=0; i<l2; i++){
						d=c;
						c = getResult(newname[i],j);
						c = d + c;
					} 
					setResult("Sum of Frequencies", j, c);
					setResult("Neighbors",j,j);
					}

				saveAs("Results", myDir2+"All_Neighbor_Values.csv");
					selectWindow("Results");
					run("Close");
				run("Close All");
		print("\\Clear");
	}	
	
		if(l2 < 2 ) { 
			if(list3.length > 1) {
			File.delete(myDir2);
			}
			print("\\Clear");
			run("Close All");
			}
		
	T2 = getTime();
		TTime = (T2-T1)/1000;

print("All Images Analyzed Successfully in:",TTime," Seconds");
exit();



