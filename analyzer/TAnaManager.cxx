#include "TAnaManager.hxx"
#include "TEnergyHistogram.h"
#include "TDeltaTimeHistogram.h"

TAnaManager::TAnaManager(){ 
   AddHistogram(new TEnergyHistogram());
   AddHistogram(new TDeltaTimeHistogram());
};

void TAnaManager::AddHistogram(THistogramArrayBase* histo) {

   histo->DisableAutoUpdate();
   //histo->CreateHistograms();
   fHistos.push_back(histo);
}


int TAnaManager::ProcessMidasEvent(TDataContainer& dataContainer){

   // Fill all the  histograms
   for (unsigned int i = 0; i < fHistos.size(); i++) {
      // Some histograms are very time-consuming to draw, so we can
      // delay their update until the canvas is actually drawn.
      if (!fHistos[i]->IsUpdateWhenPlotted()) {
        fHistos[i]->UpdateHistograms(dataContainer);
      }
   }
  
   return 1;
}

// Little trick; we only fill the transient histograms here (not cumulative), since we don't want
// to fill histograms for events that we are not displaying.
// It is pretty slow to fill histograms for each event.
void TAnaManager::UpdateTransientPlots(TDataContainer& dataContainer) {

   std::vector<THistogramArrayBase*> histos = GetHistograms();
  
   for (unsigned int i = 0; i < histos.size(); i++) {
      if (histos[i]->IsUpdateWhenPlotted()) {
         histos[i]->UpdateHistograms(dataContainer);
      }
   }
}
