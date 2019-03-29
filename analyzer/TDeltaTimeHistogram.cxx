#include "TDeltaTimeHistogram.h"
#include "TDirectory.h"
#include "TH2D.h"

/// Reset the histograms for this canvas
TDeltaTimeHistogram::TDeltaTimeHistogram() {

  SetTabName("Delta Time");
  CreateHistograms();
}

void TDeltaTimeHistogram::CreateHistograms() {

   // check if we already have histogramss.
   if(size() != 0) {
      char tname[100];
      sprintf(tname,"DeltaT_CH00_CH01");
    
      TH1I *tmp = (TH1I*)gDirectory->Get(tname);
      if (tmp) return;
   }
   
   // Otherwise make histograms
   clear();

   char name[100];
   char title[100];
   sprintf(name,"DeltaT_CH00_CH01");
   TH1D *otmp = (TH1D*)gDirectory->Get(name);
   if (otmp) delete otmp;      
   
   sprintf(title,"DeltaT histogram channels 00/01");	
      
   TH1I *tmp = new TH1I(name, title, 1000, 0, 999);
   tmp->SetXTitle("ns");
      
   push_back(tmp);
}

void TDeltaTimeHistogram::UpdateHistograms(TDataContainer& dataContainer) {

   static int time[19];

   const short unsigned int *channel = dataContainer.GetEventData<TGenericData>("CHAN")->GetData16();
   const short unsigned int *t = dataContainer.GetEventData<TGenericData>("TIME")->GetData16();
   const short unsigned int *thr = dataContainer.GetEventData<TGenericData>("TIHR")->GetData16();

   if(time[*channel] == 0) {

      time[*channel] = ((*t)*5000) + ((*thr)*300);	// picoseconds
      printf("time CH%d = %d ps\n", *channel, time[*channel]);

   } else if((*channel) && (time[*channel - 1] != 0)) {

      int value = abs(time[*channel] - time[*channel - 1]);
      GetHistogram(0)->Fill(value/1000);	// nanoseconds
      printf("delta = %d ns\n", value/1000);
      time[*channel] = time[*channel - 1] = 0;
   }
}
