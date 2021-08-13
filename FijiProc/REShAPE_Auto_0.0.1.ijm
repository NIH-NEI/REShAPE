//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------
// Modified: 11/05/2018 by Andrei I Volkov
//
// Derived from REShAPE_Manual_0.08.ijm
//--------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------

var parfile = getArgument();
print("[0] Reading arguments from: "+parfile);

requires("1.50a");
IJorFIJI = getVersion();

var param_body=File.openAsString(parfile);
replace(param_body, "\r", "");
var param_lines = split(param_body, "\n");
var param_keys = newArray(param_lines.length);
var param_values = newArray(param_lines.length);

for(i=0; i<param_lines.length; i++) {
  lns = split(param_lines[i], ";");
  ln = lns[0];
  kv = split(ln, "=");
  if (kv.length == 2) {
    param_keys[i] = kv[0];
    param_values[i] = kv[1];
  } else {
    param_keys[i] = "?";
  }
}
function getParam(parname, dflt) {
  for (i=0; i<param_keys.length; i++) {
    if (param_keys[i] == parname) return param_values[i];
  }
  return dflt;
}
function getIntParam(parname, dflt) {
  var s_val = getParam(parname, "");
  if (s_val == "") return dflt;
  return parseInt(s_val);
}

// Parameterization by A.Volkov
graphic_choice = getParam("graphic_choice", "None");
LUT_choice = getParam("LUT_choice", "Thermal");
graphic_format = getParam("graphic_format", "Tiff Stack+PNG Montage");
unit_conv = getParam("unit_conv", "Yes");
pore_lower = getParam("pore_lower", "50");
pore_upper = getParam("pore_upper", "12000");
var unit_pix = getParam("unit_pix", "1");
var unit_real = getParam("unit_real", "1");

basedir = getParam("srcDir", ".");
if (!endsWith(basedir, File.separator)) basedir = basedir + File.separator;
print("[0] SourceDir: {"+basedir+"}");

// Set up/create directories
segDir = basedir+"Segmented Images"+File.separator;
File.makeDirectory(segDir);
if (!File.exists(segDir))
	exit("Unable to create directory "+segDir);
print("[0] SegmentedDir: {"+segDir+"}");

tmpDir = basedir+"Temp"+File.separator;
File.makeDirectory(tmpDir);
if (!File.exists(tmpDir))
	exit("Unable to create directory "+tmpDir);
print("[0] TempDir: {"+tmpDir+"}");

anaDir = basedir+"Analysis"+File.separator;
File.makeDirectory(anaDir);
if (!File.exists(anaDir))
	exit("Unable to create directory "+anaDir);
print("[0] AnalysisDir: {"+anaDir+"}");

visDir = basedir+"Color Coded"+File.separator;
File.makeDirectory(visDir);
if (!File.exists(visDir))
	exit("Unable to create directory "+visDir);
print("[0] VisualizationDir: {"+visDir+"}");

	list = getFileList(basedir);
	setBatchMode(true);

	lower = 0;
	upper = 0;
	size = pore_lower + "-" + pore_upper;
	edges = "exclude";
	holes = "";
		
	T1 = getTime();	

srcFileCount = 0;
for (i=0; i<list.length; i++)
{
	basename = list[i];
	filename = basedir + basename;
	
	fext = "";
	if (endsWith(filename, ".tif"))
		fext = ".tif";
	else if (endsWith(filename, ".tiff"))
		fext = ".tiff";

	if (fext != "")
	{
		basename = replace(basename, fext, "");
		
		TC = (getTime() - T1 + 500)/1000;
		print("["+TC+"] Reading: {"+filename+"}");
		print("["+TC+"] Basename: {"+basename+"}");
		open(filename);
		
		outlines = tmpDir+basename+"_Outlines.tif";
		counted = tmpDir+basename+"_Counted.tif";
		
		// Saves B&W cell outlines for analysis
		TC = (getTime() - T1 + 500)/1000;
		print("["+TC+"] Outlines: {"+outlines+"}");		
		run("Select None");
		run("Invert");
		run("Voronoi");	
		setThreshold(lower+1, upper+255);
		setOption("BlackBackground", true);
		run("Convert to Mask");
		run("Make Binary");
		run("8-bit");
		
		saveAs("Tiff", outlines);

		run("Select None");
		run("Invert");
		if(unit_conv == "Yes") {
			run("Set Scale...", "distance="+unit_pix+" known="+unit_real+" unit=microns");
		}
		run("Set Measurements...", "area mean standard modal min centroid center perimeter bounding fit shape feret's integrated median skewness kurtosis area_fraction redirect=None decimal=3");
		imgh = getHeight();
		fimgh = 0.7573*imgh*0.04-0.1783;
		if(fimgh > 25){
			fimgh =25;
		}
		call("ij.plugin.filter.ParticleAnalyzer.setFontSize", fimgh);
		// Add " display " parameter to display "Results"
		run("Analyze Particles...", "size="+size+" pixel circularity=0.00-1.00 show=Outlines "+edges+" clear "+holes+" record");

		TC = (getTime() - T1 + 500)/1000;
		print("["+TC+"] Counted: {"+counted+"}");
		run("Select All");
		run("RGB Color");
		run("Invert");		
		saveAs("Tiff", counted);

		// Initial particle parameters - set higher precision to reduce round-off errors
		for(l=0; l<nResults; l++) {
		  Solidity = getResult("Solidity",l);
		  setResult("Solidity",l,d2s(Solidity,12));
		  Perim = getResult("Perim.",l);
		  setResult("Perim.",l,d2s(Perim,12));
		  Feret = getResult("Feret",l);
		  setResult("Feret",l,d2s(Feret,12));
		  MinFeret = getResult("MinFeret",l);
		  setResult("MinFeret",l,d2s(MinFeret,12));
		  EllipMajor = getResult("Major",l);
		  setResult("Major",l,d2s(EllipMajor,12));
		}
		
		// Initial particle data
		fn = tmpDir+basename+"_Results.csv";
		TC = (getTime() - T1 + 500)/1000;
		print("["+TC+"] Results: {"+fn+"}");
		saveAs("Results", fn);

		if(isOpen("Results")==1)
		{
			selectWindow("Results");
			run("Close");
		}
		
		TC = (getTime() - T1 + 500)/1000;
		print("["+TC+"] Done: {"+filename+"}");
		
		if(isOpen("Log")==1) { selectWindow("Log"); run("Close"); }

		run("Close All");
		srcFileCount++;
	}
}

if (srcFileCount > 1) {
	combDir = basedir+"Combined Files"+File.separator;
	File.makeDirectory(combDir);
	if (!File.exists(combDir))
		exit("Unable to create directory "+combDir);
	print("[0] CombinedDir: {"+combDir+"}");
}


T2 = getTime();
TTime = (T2-T1)/1000;

print("All Images Analyzed Successfully in:",TTime," Seconds");
exit();
