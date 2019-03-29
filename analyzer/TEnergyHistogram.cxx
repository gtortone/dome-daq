#include "TEnergyHistogram.h"
#include "TDirectory.h"
#include "TH2D.h"

const int numberChannelPerModule = 19;

/// Reset the histograms for this canvas
TEnergyHistogram::TEnergyHistogram() {

  SetTabName("Energy");
  SetSubTabName("Channel Energy");
  SetNumberChannelsInGroup(numberChannelPerModule);
  CreateHistograms();
}

void TEnergyHistogram::CreateHistograms() {

   // check if we already have histogramss.
   if(size() != 0) {
      char tname[100];
      sprintf(tname,"Energy_CH00");
    
      TH1I *tmp = (TH1I*)gDirectory->Get(tname);
      if (tmp) return;
   }
   
   // Otherwise make histograms
   clear();

   for(int i = 0; i < numberChannelPerModule; i++) { // loop over channels
      
      char name[100];
      char title[100];
      sprintf(name,"Energy_CH%02i",i);
      TH1D *otmp = (TH1D*)gDirectory->Get(name);
      if (otmp) delete otmp;      
      
      sprintf(title,"Energy histogram channel %i",i);	
      
      TH1I *tmp = new TH1I(name, title, 4096, 0, 4095);
      tmp->SetXTitle("Energy value");
      
      push_back(tmp);
    }
}

void TEnergyHistogram::UpdateHistograms(TDataContainer& dataContainer) {

   const short unsigned int *channel = dataContainer.GetEventData<TGenericData>("CHAN")->GetData16();
   const short unsigned int *energy = dataContainer.GetEventData<TGenericData>("ENER")->GetData16();

   GetHistogram(*channel)->Fill(*energy);
}
