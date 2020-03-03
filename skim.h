#include "ROOT/RDataFrame.hxx"
#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RVec.hxx"

#include "Math/Vector4D.h"
#include "TStopwatch.h"

#include <stdlib.h>
#include <string>
#include <vector>
#include <iostream>
#include <cmath>

#include "utility" // std::pair
#include <algorithm> // for std::find
#include <iterator> // for std::begin, std::end

#include "TRandom3.h"
#include "TLorentzVector.h"

namespace Helper {

  /*
   * sort TnP electron candidates in pT descending order
   */
  struct pTSorter {
    bool operator() (std::pair<int,float> i , std::pair<int,float> j) { return ( (i.second) > (j.second) ); }
  };

  //template <typename T>
  //std::vector<std::pair<int,float>> IndexBypT(T v){
  //  pTSorter comparator;
  //  std::sort (v.begin() , v.end() , comparator);
  //  return v;
  //}

  template <typename T>
  std::vector<int> IndexBypT(T v){
    pTSorter comparator;
    std::vector<int> indecies;
    std::sort (v.begin() , v.end() , comparator);
    for (auto it = v.begin() ; it != v.end() ; ++it){
      indecies.push_back((*it).first);
    }
    return indecies;
  }

  /*
   * Compute the difference in the azimuth coordinate taking the boundary conditions at 2*pi into account.
   */
  template <typename T>
  float DeltaPhi(T v1, T v2, const T c = M_PI)
    {
      auto r = std::fmod(v2 - v1, 2.0 * c);
      if (r < -c) {
	r += 2.0 * c;
      }
      else if (r > c) {
	r -= 2.0 * c;
      }
      return r;
    }
}

/*
 * EDFilter
 * goodElectron
 */
template <typename T>
auto Filterbaseline(T &df) {
  return df.Define("isMC", "run==1")
           .Filter("nElectron>0"," --> At least one electron")
           ;
}

/*************************************************** tag_sequence ***************************************************/

/*
 * EDFilter
 * Kinematically good electrons
 * https://github.com/cms-analysis/EgammaAnalysis-TnPTreeProducer/blob/master/python/egmGoodParticlesDef_cff.py#L75-L79
 */
template <typename T>
auto goodElectrons(T &df) {
  return df.Define("goodElectrons","abs(Electron_eta) < 2.5 && Electron_pt > 5")
    .Filter("Sum(goodElectrons==0)==0"," --> ELECTRON_CUTS : the baseline cuts");
}

/*
 * EDProducer
 * Kinematically good jets
 */
template <typename T>
auto goodJets(T &df) {
  return df.Define("goodJets","Jet_pt > 30 && abs(Jet_eta) < 2.5 && Jet_jetId > 0 && Jet_puId > 4");
}

/*
 * EDProducer
 * clean electron from jets
 * Bool Vector
 */
template <typename T>
auto cleanFromJet(T &df) {
  using namespace ROOT::VecOps;
  auto cleanFromJet = [](RVec<int>& goodElectron, RVec<float>& eta_1, RVec<float>& phi_1,
                       RVec<int>& goodJet, RVec<float>& eta_2, RVec<float>& phi_2)
    {

      // Get indices of all possible combinations
      auto comb = Combinations(eta_1,eta_2);
      const auto numComb = comb[0].size();
      // Find match pair and eliminate the electron
      RVec<int> CleanElectron(eta_1.size(),0);
      for (size_t i = 0 ; i < numComb ; i++) {
        const auto i1 = comb[0][i];
        const auto i2 = comb[1][i];
        if( goodElectron[i1] == 1 && goodJet[i2] == 1 ){
          const auto deltar = sqrt(
				   pow(eta_1[i1] - eta_2[i2], 2) +
				   pow(Helper::DeltaPhi(phi_1[i1], phi_2[i2]), 2));
          if (deltar > 0.3) {
	    CleanElectron[i1] = 1; // no match, good electron
	  }
	}
      }

      return CleanElectron;
    };

  return df.Define("cleanFromJet", cleanFromJet,
		   {"goodElectrons", "Electron_eta", "Electron_phi",
                       "goodJets", "Jet_eta" , "Jet_phi"});
}

/*
 * EDProducer
 * find tag ele cutbased
 * https://github.com/cms-analysis/EgammaAnalysis-TnPTreeProducer/blob/master/python/egmElectronIDModules_cff.py#L144-L150
 * egmGsfElectronIDs:cutBasedElectronID-Fall17-94X-V2-tight
 */

template<typename T>
auto tagEleCutBasedTight(T &df) {
  return df.Define("tagEleCutBasedTight","goodElectrons==1 && cleanFromJet==1 && Electron_cutBased==4");
}

/*
 * EDProducer
 * tagEle
 * https://github.com/cms-analysis/EgammaAnalysis-TnPTreeProducer/blob/master/python/egmTreesSetup_cff.py#L29-L37
 */
template<typename T>
auto tagEle(T &df) {
  using namespace ROOT::VecOps;
  // lambda function
  auto tagEle = [](RVec<int>& tagEleCutBasedTight , RVec<int>& id, RVec<int>& filterBits , RVec<float>& eta_1 , RVec<float>& phi_1 , RVec<float>& eta_2 , RVec<float>& phi_2)
    {
      // get combination
      auto comb = Combinations(eta_2,eta_1); // electron-trig object pair
      RVec<int> MatchElectron(eta_2.size(),0);
      const auto numComb = comb[0].size();
      for (size_t i=0 ; i < numComb ; i++){
        const auto i1 = comb[0][i]; //electron
        const auto i2 = comb[1][i]; //trigobj
        if (tagEleCutBasedTight[i1] != 1) continue;
        if (abs(id[i2])!=11) continue;
        // hltEle*WPTight*TrackIsoFilter* --> HLT Ele27 eta2p1 WPTight Gsf v*
        // https://github.com/cms-sw/cmssw/blob/master/PhysicsTools/NanoAOD/python/triggerObjects_cff.py#L35-L46
        if ( filterBits[i2] & ( 1 << 1 ) ) {
          //if (pt_2[i1] < 27) continue;
          //if (abs(eta_2[i1]) > 2.1) continue;
          const auto deltar = sqrt(
                                   pow(eta_1[i1] - eta_2[i2], 2) +
                                   pow(Helper::DeltaPhi(phi_1[i1], phi_2[i2]), 2));
          if (deltar < 0.3) {
            MatchElectron[i1] = 1;
          }
        }
      }

      return MatchElectron;
    };
  return df.Define("tagEle",tagEle,{"tagEleCutBasedTight","TrigObj_id","TrigObj_filterBits","TrigObj_eta","TrigObj_phi","Electron_eta","Electron_phi"});
    //.Filter("Sum(triggerMatchedElectron==1)>=1"," --> Event has at least one electron match to trigger");
}

/*
 * EDFilter + EDProducer
 * genEle + genTagEle + genProbeEle
 * https://github.com/cms-analysis/EgammaAnalysis-TnPTreeProducer/blob/master/python/egmTreesSetup_cff.py#L83-L86
 */

 template<typename T>
 auto genTagEle(T &df, bool& isMC) {
  using namespace ROOT::VecOps;
  auto genTagEle = [](RVec<int>& tagEle , RVec<float>& ele_eta , RVec<float>& ele_phi ,
    RVec<int>& gen_pdgId , RVec<float>& gen_pt , RVec<float>& gen_eta , RVec<float>& gen_phi , RVec<int>& statusflag) {
    auto comb = Combinations(ele_eta,gen_eta);
    const auto numComb = comb[0].size();

    // genmatch
    RVec<int> tagEle_GenEleIdx(numComb,-1);

    for (size_t i = 0 ; i < numComb ; i++) {
      const auto i1 = comb[0][i]; // tagEle
      const auto i2 = comb[1][i]; // genTagEle
      if (tagEle[i1]!=1) continue;
      if (abs(gen_pdgId[i2]) != 11) continue; // do not check charge
      if (gen_pt[i2] < 3) continue;
      if (abs(gen_eta[i2]) > 2.7) continue;
      float mindr= 99999.;
      // isPromptFinalState = isPrompt + isLastCopy
      // resolve ambiguity, pick lowest deltaR pair first
      if ( (statusflag[i2] & ( 1 << 0 )) && (statusflag[i2] & ( 1 << 13 ))  ){
        const auto deltar = sqrt(
                                 pow(ele_eta[i1] - gen_eta[i2], 2) +
                                 pow(Helper::DeltaPhi(ele_phi[i1], gen_phi[i2]), 2));
        if (mindr > deltar ) {
          mindr = deltar;
        }
      }
      if (mindr < 0.2){
        tagEle_GenEleIdx[i] = i2;
      }
    } // end numComb loop
    return tagEle_GenEleIdx;
  }; // end lambda function
  if (!isMC){
    return df.Define("genTagEle","tagEle")
             .Define("genProbeEle","tagEleCutBasedTight==1");
  }
  else
    return df.Define("genTagEle",genTagEle,{"tagEle","Electron_eta","Electron_phi","GenPart_pdgId","GenPart_pt","GenPart_eta","GenPart_phi","GenPart_statusFlags"})
             .Define("genProbeEle","tagEleCutBasedTight==1");
  }


/*************************************************** ele_sequence ***************************************************/

/*
 * EDProducer
 * cms.sequence of WPs
 * https://github.com/cms-analysis/EgammaAnalysis-TnPTreeProducer/blob/master/python/egmTreesSetup_cff.py#L172-L205
 */

template <typename T>
auto WPsequence(T &df) {

  return df.Define("probeEleCutBasedVeto"   , "goodElectrons==1 && cleanFromJet==1 && Electron_cutBased==1")
           .Define("probeEleCutBasedLoose"  , "goodElectrons==1 && cleanFromJet==1 && Electron_cutBased==2")
           .Define("probeEleCutBasedMedium" , "goodElectrons==1 && cleanFromJet==1 && Electron_cutBased==3")
           .Define("probeEleCutBasedTight"  , "goodElectrons==1 && cleanFromJet==1 && Electron_cutBased==4")
           .Define("probeEleMVAtth"         , "goodElectrons==1 && cleanFromJet==1 && Electron_mvaTTH>0.7")
           .Define("probeEle"               , "tagEleCutBasedTight==1");
}

/*************************************************** tnpPairingEleIDs ***************************************************/
/*
 * EDProducer + EDAnalyzer
 * tnpPairingEleIDs
 * https://github.com/cms-analysis/EgammaAnalysis-TnPTreeProducer/blob/master/python/egmTreesSetup_cff.py#L105-L111
 * pair collection
 */
template <typename T>
auto tnpPairingEleIDs(T &df) {
  using namespace ROOT::VecOps;
  auto tnpPairingEleIDs = [](RVec<int>& tagEle_flag, RVec<int>& probeEle_flag , RVec<float>& pt , RVec<float>& eta , RVec<float>& phi , RVec<float>& m){

  auto comb = Combinations(tagEle_flag,probeEle_flag);
  const auto tagcomb = comb[0].size();
  RVec<std::pair<int,int>> TnPCandidate;

  for(size_t i = 0; i < tagcomb; i++) {
    const auto tag = comb[0][i];
    const auto probe = comb[1][i];

    if (tagEle_flag[tag] != 1) continue;
    if (probeEle_flag[probe] != 1) continue;

    TLorentzVector tot, LVprobe;
    tot.SetPtEtaPhiM( pt[tag] , eta[tag] , phi[tag] , m[tag] );
    LVprobe.SetPtEtaPhiM( pt[probe] , eta[probe] , phi[probe] , m[probe] );
    tot+=LVprobe;
    if ( tot.M() < 50 && tot.M() > 130 ) continue;
    TnPCandidate.push_back(std::make_pair(tagEle_flag[tag],probeEle_flag[probe]));
 }
   return TnPCandidate;
}; // end of lambda function

return df.Define("tnpPairingEleIDs", tnpPairingEleIDs, {"tagEle","probeEle","Electron_pt","Electron_eta","Electron_phi","Electron_mass"});
  }

/*
 * EDAnalyzer
 * TagProbeFitTreeProducer
 * InputTag = tnpPairingEleIDs, genProbeEle, probeEle
 * https://github.com/cms-analysis/EgammaAnalysis-TnPTreeProducer/blob/master/python/TnPTreeProducer_cfg.py#L265-L307
 */

template <typename T>
auto tnpEleIDs(T &df) {
  using namespace ROOT::VecOps;
  auto tnpEleIDs = [](RVec<std::pair<int,int>>&  tnpPairingEleIDs , RVec<int>& genTagEle , RVec<int>& genProbeEle){

    const auto num = tnpPairingEleIDs.size();
    TRandom3 rand(0);
    bool valid=false;
    int irand, tidx = -1, pidx = -1;

    // throw random number on the tag-probe pair, if the pair is the same, reselect (since no resolving ambiguity in making pair)
    while(!valid){
      irand = static_cast<int>(rand.Uniform(0,num)*10) % 2;
      std::cout<<"throwing random number : "<<irand<<std::endl;
      tidx = tnpPairingEleIDs[irand].first;
      pidx = tnpPairingEleIDs[irand].second;
      if ( tidx != pidx ){
        std::cout<<"break while loop"<<std::endl;
        valid=true;
      }
    }
    // data by default is true
    int mcTrue = genTagEle[tidx] && genProbeEle[pidx];
    return std::vector<int>({tidx , pidx , mcTrue});
  };
return df.Define("tnpEleIDs", tnpEleIDs, {"tnpPairingEleIDs","genTagEle","genProbeEle"})
         .Define("tag_Idx", "tnpEleIDs[0]")
         .Define("probe_Idx","tnpEleIDs[1]")
         .Define("mcTruth","tnpEleIDs[2]")
         // WP
         .Define("passingVeto","probeEleCutBasedVeto[probe_Idx]")
         .Define("passingLoose","probeEleCutBasedLoose[probe_Idx]")
         .Define("passingMedium","probeEleCutBasedMedium[probe_Idx]")
         .Define("passingTight","probeEleCutBasedTight[probe_Idx]")
         .Define("passingMVAtth","probeEleMVAtth[probe_Idx]");
}

/*
 * Declare variables for analysis
 */

template <typename T>
auto DeclareVariables(T &df) {

  auto add_p4 = [](float pt, float eta, float phi, float mass)
    {
      return ROOT::Math::PtEtaPhiMVector(pt, eta, phi, mass);
    };

  auto pair_kin = [](ROOT::Math::PtEtaPhiMVector& p4_1, ROOT::Math::PtEtaPhiMVector& p4_2)
    {
      return std::vector<float>( { float((p4_1+p4_2).Pt()) , float((p4_1+p4_2).Eta()) , float((p4_1+p4_2).Phi()) , float((p4_1+p4_2).M()) } );
    };

  return df
    .Define("tag_Ele_pt" ,"Electron_pt[tag_Idx]")
    .Define("tag_Ele_eta","Electron_eta[tag_Idx]")
    .Define("tag_Ele_phi","Electron_phi[tag_Idx]")
    .Define("tag_Ele_mass","Electron_mass[tag_Idx]")
    .Define("tag_Ele_q","Electron_charge[tag_Idx]")
    .Define("tag_Ele",add_p4,{"tag_Ele_pt","tag_Ele_eta","tag_Ele_phi","tag_Ele_mass"})

    .Define("probe_Ele_pt" ,"Electron_pt[probe_Idx]")
    .Define("probe_Ele_eta","Electron_eta[probe_Idx]")
    .Define("probe_Ele_phi","Electron_phi[probe_Idx]")
    .Define("probe_Ele_mass","Electron_mass[probe_Idx]")
    .Define("probe_Ele_q","Electron_charge[probe_Idx]")
    .Define("probe_Ele",add_p4,{"probe_Ele_pt","probe_Ele_eta","probe_Ele_phi","probe_Ele_mass"})

    .Define("pair_kin" ,pair_kin,{"tag_Ele","probe_Ele"})
    .Define("pair_pt" ,"pair_kin[0]")
    .Define("pair_eta" ,"pair_kin[1]")
    .Define("pair_phi" ,"pair_kin[2]")
    .Define("pair_m" ,"pair_kin[3]");
}

/*
 * Add event weights
 */

template <typename T>
auto AddEventWeight(T &df, const std::string& path, const std::string& sample, const std::string& lumi, const std::string& weight1, const std::string& weight2) {
  std::string weights;
  if (path.find("DATA") != std::string::npos) {
    weights = "METFilter_DATA*"+weight1+"*"+weight2;
  }
  else {
    weights = lumi+"*XSWeight*PrefireWeight*puWeight*GenLepMatch2l*METFilter_MC*("+weight1+")*("+weight2+")";
  }
  std::cout<<" weights interpreted : "<<weights<<std::endl;
  return df.Define( "weight", weights );
}

/*
 * Declare all variables which will end up in the final reduced dataset
 */

const std::vector<std::string> finalVariables = {
  "tag_Ele_pt" , "tag_Ele_eta" , "tag_Ele_phi" , "probe_Ele_pt", "probe_Ele_eta" ,
  "probe_Ele_phi" , "weight", "pair_pt" , "pair_eta" , "pair_phi" , "pair_mass" ,
  "passingCutBasedIDFall17V2" , "passingmvaTTH" , "tagEle" , "probeEle" , "nElectron"
};
