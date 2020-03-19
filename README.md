# NanoTnP
minimalistic TnP package on nanoAod.

## Tag-and-Probe Methodology 
The package produces the necessary ntuple format for identification efficiency of electrons. 
The tag is a well identified and isolated electron which pass an electron trigger to increase the purity. Once the tag electron is selected then we look for another object which is reconstructed as electron. After passing some kinematical selection, we get the total probe selection. The invariant mass of the pair is then reconstructed from Tag and Probe electron and a Z boson mass window is imposed. We ask the Probe to pass the ID working point and compute the efficiency. Once we obtain the efficiency for both DATA and MC then we can compute the scale factor, which is the ratio of efficiencies of DATA and MC.

## Brief technical aspect
The package constructed in plug-in-play modular nature, in EDM's language, a series of EDMProducer/EDMFilter parameterized with PSet before EDAnalyzer, string in cms sequence. Thanks to RDataframe, module is implemented as collective transformation fucntions (lambda function implemented to compute particle-based quantity) which lazily executed upon action. Benefits are, speed and better cpu utilization.

 - Samples used for TnP defined in `NanoTnP\data\latinov5_17`, `NanoTnP\data\latinov5_18`
 - Execution script:  `NanoTnP/nanotnp.py`
 - Output definition: `NanoTnP/interface/config.h`
 - Skim definition:   `NanoTnP/interface/skim.h`
 - Producer definition: `NanoTnP/interface/producer.h`
 - Sequence definition: `NanoTnP/nanotnp.cxx`
 
Default definition
 
| Object Definition | Description |
| --- | --- |
| Tag (data) | Match to last trigger filter, with stricter selection. |
| Tag (mc) | Match to Gen-level lepton which originating from Z boson, with stricter selection. |
| Probe (data,mc) | no matching |
| tag-probe pair (data,mc) | probe pair with tag within Z mass window, no charge check, randomize pair definition |
| Weight (data) | METFilter_DATA |
| Weight (mc) | LUMI * XSWeight * puWeight * GenLepMatch2l * METFilter_MC |
 
 
 ## Setup
 ROOT > 6.18 ; python3.7 ; on lxplus7
 
 ```
 source /cvmfs/sft.cern.ch/lcg/views/LCG_95/x86_64-centos7-gcc8-opt/setup.sh
 git clone git@github.com:SiewYan/NanoTnP.git
 cd NanoTnp
 # for latinov5 2017
 python nanotnp.py -y latinov5_17 
 # for latinov5 2018
 python nanotnp.py -y latinov5_18 
 ```
  - default at 10 cores for parallelism
  - 2017's approximately takes ~ 13 mins (data, mc, altmc)
  - 2018's approximately takes ~ 19 mins (data, mc, altmc)
  
  A `results` folder will appear containing the output.
 
 ## To-do
 - [x] mcTruth for tag only... needed probe is mcTruth requirement ?
 - [ ] missing trigger filter for 2017, `hltEle35noerWPTightGsfTrackIsoFilter`.
 - [ ] implement `PromptGenLepton2l`.
 - [x] json selection on data for now.
 
 # Egm Analysis 
 Copied from https://github.com/swagata87/egm_tnp_analysis
 ## Quick description
on lxplus (do not use cmsenv from cmssw)

Package to handle analysis of tnp trees. The main tool is the python fitter

   ===> tnpEGM_fitter.py

The interface between the user and the fitter is solely done via the settings file

   ===> etc/config/settings.py
   	- set the flags (i.e. Working points) that can be tested
   	- set the different samples and location
	- set the fitting bins
	- set the different cuts to be used
	- set the output directory

Help message:
>    python tnpEGM_fitter.py --help 

The settings have always to be passed to the fitter
>    python tnpEGM_fitter.py etc/config/settings.py 

Therefore several "settings.py" files  can be setup (for different run period for instance)


## The different fitting steps
Everything will be done for a specific flag (so the settings can be the same for different flags). Hence, the flag to be used must be specified each time (named myWP in following).

**1. Create the bining.** To each bin is associated a cut that can be tuned bin by bin in the settings.py
   * After setting up the settings.py check bins 

>   python tnpEGM_fitter.py settings.py --checkBins
   
   * if  you need additinal cuts for some bins (cleaning cuts), tune cuts in the settings.py, then recheck. 
     Once satisfied, create the bining

>   python tnpEGM_fitter.py settings.py --createBins

   * CAUTION: when recreacting bins, the output directory is overwritten! So be sure to not redo that once you are at step2

**2. Create the histograms** with the different cuts... this is the longest step. Histograms will not be re-done later
   
>   python tnpEGM_fitter.py settings.py --createHists

**3. Do your first round of fits.**
   * nominal fit
   
>   python tnpEGM_fitter.py settings.py --doFit
   
   * MC fit to constrain alternate signal parameters [note this is the only MC fit that makes sense]
   
>   python tnpEGM_fitter.py settings.py --doFit --mcSig --altSig

   * Alternate signal fit (using constraints from previous fits)
   
>   python tnpEGM_fitter.py settings.py --doFit  --altSig

   * Alternate background fit (using constraints from previous fits)
   
>   python tnpEGM_fitter.py settings.py --doFit  --altBkg

**4. Check fits and redo failed ones.** (there is a web index.php in the plot directory to vizualize from the web)
   * can redo a given bin using its bin number ib. 
     The bin number can be found from --checkBins, directly in the ouput dir (or web interface)

>   python tnpEGM_fitter.py settings.py --doFit --iBin ib
   
   * the initial parameters can be tuned for this particular bin in the settings.py file. 
      Once the fit is good enough, do not redo all fits, just fix next failed fit.
      One can redo any kind of fit bin by bin. For instance the MC with altSig fit (if the constraint parameters were bad in the altSig for instance)

>   python tnpEGM_fitter.py settings.py --doFit --mcSig --altSig --iBin ib

**5. egm txt ouput file.** Once all fits are fine, put everything in the egm format txt file

>   python tnpEGM_fitter.py setting.py --sumUp
   

## The settings file

The settings file includes all the necessary information for a given setup of fit

**- General settings.**

    * flag: this is the Working point in the tnpTree  (pass: flagCut ; fail !flagCut). The name of the flag myWP is the one to be passed
to the fitter. One can handle complex flags with a cut string (root cut string):
> flag = { 'myWP' : myWPCutString } 

    * baseOutDir: the output directory (will be created by the fitter)

**- Sample definition.**

    * tnpTreeDir: the directory in the tnpTree (different for phoID, eleID, reco, hlt)

    * samplesDef: these are the main info
      - data: data ntuple
      - mcNom: nominal MC sample
      - mcAlt: MC for generator syst
      - tagSel: usually same as nominal MC + different base cuts: check the tag selection syst

     The sample themselves are defined in etc/inputs/tnpSampleDef.py  (the attribute nEvts, lumi are not necessary for the fit per-se and can be omitted). 
     A list of samples for ICHEP2016 from official egm production is already setup properly in the package. 
     Then in the settings.py the sample can be specified further:
     - sample.set_mctruth() : force mc truth on a MC sample
     - sample.rename('xxx') : if a sample is used 2 times (like with 2 different sets of cuts), it has to be renamed for the second use
     - sample.set_cut(cut)  : add a cut to define the sample (like a run range for data or an additional tag selection for alt tag selection syst)
     - sample.set_weight('totWeight') : name of the weight to be used for MC reweighting (totWeight in this example). Note: the tool can handle a pu Tree to reweight a MC with different PU scenario (ask for further explanations and/or settings_rwPU.py example)
 

**- Cuts.**

    * cutBase: Define here the main cut
    * additionalCuts: can be used for cleaning cuts (or put additionalCuts = None)

**- Fitting parameters.**
    
    Define in this section the init parameters for the different fit, can be tuned to improve convergence.

====================
   


##  Update PU weights 

1. Pileup files have to be computed with: python etc/scripts/pureweight.py

Here one has to update the name of the directory whre the files will be located and the corresponding names.


2.This pyhton uses the following: libPython/puReweighter.py.

Here one nees to add the PU MC mix numbers that are available here: http://cmslxr.fnal.gov/source/SimGeneral/MixingModule/python/?v=CMSSW_9_4_0

One also needs to update sample names here:
etc/inputs/tnpSampleDef.py

3.The data PU distrubtions can be computed using the following instructions (similar to what is done in step1):

pileupCalc.py -i /afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions17/13TeV/PromptReco/Cert_294927-306462_13TeV_PromptReco_Collisions17_JSON.txt --inputLumiJSON /afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions17/13TeV/PileUp/pileup_latest.txt --calcMode true --minBiasXsec 69200 --maxPileupBin 100 --numPileupBins 100 pileup_2017_41fb.root

Other pu files for each run, like pileup_2017_RUNB.root, pileup_2017_RUNC.root etc, can be copied from previous location. The previous location of pu directory can be found in github. For example, in this version, the location is,  
/eos/cms/store/group/phys_egamma/swmukher/tnp/ID_V2_2017/PU


The nvtx and rho histos are not needed because we will use the pu method (type = 0) for the reweight.

NB: Before using these py in order to load the needed libraires one has to run: 
export  PYTHONPATH=$PYTHONPATH:/afs/cern.ch/user/s/soffi/scratch0/TEST/CMSSW-10-0-0-pre3/src/egm_tnp_analysis 


#### adding remote (Fabrice version)
git remote add origin git@github.com:fcouderc/egm_tnp_analysis.git
