//**************************************************
// \file PhysicsConverter.C
// \brief: converter from merged trees to Event obj
// \author: Lorenzo Pezzotti (CERN EP-SFT-sim) @lopezzot
//          Edoardo Proserpio (Uni Insubria)
// \start date: 20 August 2021
//**************************************************
//
////usage: root -l .x PhysicsConverter.C++
//
//
#include <TTree.h>
#include <TFile.h>
#include <TString.h>
#include <TProfile.h>
#include <iostream>
#include <array>
#include <stdint.h>
#include <string>
#include <fstream>
#include "PhysicsEvent.h"
#include <string>
#include <cstring>


using json = nlohmann::json;

ClassImp(EventOut)

void PhysicsConverter(const string run, const string inputPath, const string calFile){

  //Open merged ntuples
  //
  string infile = inputPath+"merged_sps2023_run"+run+".root"; // Actually we can also merge the merged_sps2023_run into inputPath
  //infile = inputPath+"output"+run+".root";
  std::cout<<"Using file: "<<infile<<std::endl;
  char cinfile[infile.size() + 1];
  strcpy(cinfile, infile.c_str());
  string outfile = "physics_sps2023_run"+run+".root"; // Make sure this matches the mv file in DoPhysicsConverter.py
  char coutfile[outfile.size() + 1];
  strcpy(coutfile, outfile.c_str());
  auto Mergfile = new TFile(cinfile, "READ");
  auto *PMTtree = (TTree*) Mergfile->Get("CERNSPS2023");
  auto *SiPMtree = (TTree*) Mergfile->Get("SiPMSPS2023");
  //Create new tree and Event object
  //
  TFile * Outfile = new TFile(coutfile,"RECREATE");
  TTree * ftree = new TTree("Ftree","Ftree");
  ftree->SetDirectory(Outfile);


  //Create calibration objects
  //
  SiPMCalibration sipmCalibration(calFile);
  PMTCalibration pmtCalibration(calFile);
  DWCCalibration dwcCalibration(calFile);

  //Check entries in trees
  //

  unsigned int nentries = PMTtree->GetEntries();

  std::cout<<"Entries in PMT / SiPM tree "<< nentries <<" / "<<SiPMtree->GetEntries()<<std::endl;

  //Allocate branch pointers
  //
  int EventID;
  PMTtree->SetBranchAddress("EventNumber",&EventID);
  int ADCs[64];
  PMTtree->SetBranchAddress("ADCs",&ADCs);
  int TDCsval[48];
  PMTtree->SetBranchAddress("TDCsval",&TDCsval);
  Long64_t TriggerMask;
  PMTtree->SetBranchAddress("TriggerMask",&TriggerMask);

  Event * ev = new Event();
  EventOut  * evout = new EventOut();
  ftree->Branch("Events",evout);

  // Determine the PMT pedestals
    
  std::vector<TProfile *> h_ped_scin;
  std::vector<TProfile *> h_ped_cher;

  TProfile * h_ped_PShower = 0; 
  TProfile * h_ped_MCounter = 0;
  TProfile * h_ped_C1 = 0;
  TProfile * h_ped_C2 = 0;
  TProfile * h_ped_C3 = 0;
  
  
  // There is a pedestal event every 10 (actually 11) events, and 
  // we probably want a granularity at the level of 100 events or so. 
  // Also, we don't have runs much longer than 50k events, so, the number below shoudl be reasonable. 
  
  unsigned int nbins = (nentries / 100);
  TString s_ped_scin = "h_ped_scin";
  TString s_ped_cher = "h_ped_cher";
  
  for (unsigned int ch = 0; ch < 8; ++ch){
    h_ped_scin.push_back(new TProfile(s_ped_scin + ch,"",nbins,0.,(Float_t) nentries));
    h_ped_cher.push_back(new TProfile(s_ped_cher + ch,"",nbins,0.,(Float_t) nentries));
  }

  ev->m_h_ped_scin = &h_ped_scin;
  ev->m_h_ped_cher = &h_ped_cher;

  h_ped_PShower = new TProfile("h_ped_PShower","",nbins,0.,(Float_t) nentries);
  h_ped_MCounter = new TProfile("h_ped_MCounter","",nbins,0.,(Float_t) nentries);
  h_ped_C1 = new TProfile("h_ped_C1","",nbins,0.,(Float_t) nentries);
  h_ped_C2 = new TProfile("h_ped_C2","",nbins,0.,(Float_t) nentries);
  h_ped_C3 = new TProfile("h_ped_C3","",nbins,0.,(Float_t) nentries);

  // First loop - determined the pedestals

  for (unsigned int i = 0; i < PMTtree->GetEntries(); ++i){
    PMTtree->GetEntry(i);
    if (TriggerMask == 6 /* it is a pedestal event */){
      h_ped_scin[0]->Fill(float(i),float(ADCs[8]));
      h_ped_scin[1]->Fill(float(i),ADCs[9]);
      h_ped_scin[2]->Fill(float(i),ADCs[10]);
      h_ped_scin[3]->Fill(float(i),ADCs[11]);
      h_ped_scin[4]->Fill(float(i),ADCs[12]);
      h_ped_scin[5]->Fill(float(i),ADCs[13]);
      h_ped_scin[6]->Fill(float(i),ADCs[14]);
      h_ped_scin[7]->Fill(float(i),ADCs[15]);
      h_ped_cher[0]->Fill(float(i),ADCs[0]);
      h_ped_cher[1]->Fill(float(i),ADCs[1]);
      h_ped_cher[2]->Fill(float(i),ADCs[2]);
      h_ped_cher[3]->Fill(float(i),ADCs[3]);
      h_ped_cher[4]->Fill(float(i),ADCs[4]);
      h_ped_cher[5]->Fill(float(i),ADCs[5]);
      h_ped_cher[6]->Fill(float(i),ADCs[6]);
      h_ped_cher[7]->Fill(float(i),ADCs[7]);
      h_ped_PShower->Fill(float(i),ADCs[16]);
      h_ped_MCounter->Fill(float(i),ADCs[32]);
      h_ped_C1->Fill(float(i),ADCs[33]);
      h_ped_C2->Fill(float(i),ADCs[36]);
      h_ped_C3->Fill(float(i),ADCs[35]);
    }
  }



  // Now dealing with the new tree

  SiPMtree->SetBranchAddress("HG_Board0",&ev->SiPMHighGain[0]);
  SiPMtree->SetBranchAddress("HG_Board1",&ev->SiPMHighGain[64]);
  SiPMtree->SetBranchAddress("HG_Board2",&ev->SiPMHighGain[128]);
  SiPMtree->SetBranchAddress("HG_Board3",&ev->SiPMHighGain[192]);
  SiPMtree->SetBranchAddress("HG_Board4",&ev->SiPMHighGain[256]);
  SiPMtree->SetBranchAddress("LG_Board0",&ev->SiPMLowGain[0]);
  SiPMtree->SetBranchAddress("LG_Board1",&ev->SiPMLowGain[64]);
  SiPMtree->SetBranchAddress("LG_Board2",&ev->SiPMLowGain[128]);
  SiPMtree->SetBranchAddress("LG_Board3",&ev->SiPMLowGain[192]);
  SiPMtree->SetBranchAddress("LG_Board4",&ev->SiPMLowGain[256]);



  //Loop over events 
  //
  for( unsigned int i=0; i<PMTtree->GetEntries(); ++i){
    PMTtree->GetEntry(i);
    SiPMtree->GetEntry(i);
    evout->EventID = EventID;
    evout->TriggerMask = TriggerMask;

    //Fill ev data members
    //
    ev->SPMT1 = ADCs[8];
    ev->SPMT2 = ADCs[9];
    ev->SPMT3 = ADCs[10];
    ev->SPMT4 = ADCs[11];
    ev->SPMT5 = ADCs[12];
    ev->SPMT6 = ADCs[13];
    ev->SPMT7 = ADCs[14];
    ev->SPMT8 = ADCs[15];
    ev->CPMT1 = ADCs[0];
    ev->CPMT2 = ADCs[1];
    ev->CPMT3 = ADCs[2];
    ev->CPMT4 = ADCs[3];
    ev->CPMT5 = ADCs[4];
    ev->CPMT6 = ADCs[5];
    ev->CPMT7 = ADCs[6];
    ev->CPMT8 = ADCs[7];
    evout->SPMT1_adc = float(ADCs[8]);
    evout->SPMT2_adc = float(ADCs[9]);
    evout->SPMT3_adc = float(ADCs[10]);
    evout->SPMT4_adc = float(ADCs[11]);
    evout->SPMT5_adc = float(ADCs[12]);
    evout->SPMT6_adc = float(ADCs[13]);
    evout->SPMT7_adc = float(ADCs[14]);
    evout->SPMT8_adc = float(ADCs[15]);
    evout->CPMT1_adc = float(ADCs[0]);
    evout->CPMT2_adc = float(ADCs[1]);
    evout->CPMT3_adc = float(ADCs[2]);
    evout->CPMT4_adc = float(ADCs[3]);
    evout->CPMT5_adc = float(ADCs[4]);
    evout->CPMT6_adc = float(ADCs[5]);
    evout->CPMT7_adc = float(ADCs[6]);
    evout->CPMT8_adc = float(ADCs[7]);
    evout->PShower = ADCs[16];
    evout->PShower_ped = ev->getPedestal(h_ped_PShower,i);
    evout->MCounter = ADCs[32];
    evout->MCounter_ped = ev->getPedestal(h_ped_MCounter,i);
    evout->C1 = ADCs[33];
    evout->C1_ped = ev->getPedestal(h_ped_C1,i);
    evout->C2 = ADCs[36];
    evout->C2_ped = ev->getPedestal(h_ped_C2,i);
    evout->C3 = ADCs[35];
    evout->C3_ped = ev->getPedestal(h_ped_C3,i);
   
    //
    ev->DWC1L=TDCsval[0];
    ev->DWC1R=TDCsval[1];
    ev->DWC1U=TDCsval[2];
    ev->DWC1D=TDCsval[3];
    ev->DWC2L=TDCsval[4];
    ev->DWC2R=TDCsval[5];
    ev->DWC2U=TDCsval[6];
    ev->DWC2D=TDCsval[7];

    //Calibrate SiPMs and PMTs
    //
    ev->calibrate(sipmCalibration, evout);
    ev->calibratePMT(pmtCalibration, evout,i);
    ev->calibrateDWC(dwcCalibration, evout);
    evout->CompSPMTene();
    evout->CompCPMTene();
    //std::cout<<ev->EventID<<" "<<ev->totSiPMPheS<<std::endl;
    //Write event in ftree
    //
    ftree->Fill();
    //Reset totSiPMPheC and totSiPMPheS to 0
    //
    evout->totSiPMCene = 0;
    evout->totSiPMSene = 0;
  }

  //Write and close Outfile
  //
  Mergfile->Close();
  Outfile->cd();
  Outfile->mkdir("Pedestal_Histograms");
  Outfile->cd("Pedestal_Histograms");
  for (unsigned int ch = 0; ch < 8; ++ch){
    h_ped_scin[ch]->Write();
    h_ped_cher[ch]->Write();
  }
  h_ped_PShower->Write();
  h_ped_MCounter->Write();
  h_ped_C1->Write();
  h_ped_C2->Write();
  h_ped_C3->Write();
  Outfile->cd();  
  ftree->Write("",TObject::kOverwrite);
  Outfile->Close();

}

//**************************************************

