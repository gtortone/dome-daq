#ifndef TAnaManager_h
#define TAnaManager_h

// Use this list here to decide which type of equipment to use.

#include "THistogramArrayBase.h"

// manager class; holds different set of his
class TAnaManager  {

public:
  TAnaManager();
  virtual ~TAnaManager(){};

  /// Processes the midas event, fills histograms, etc.
  int ProcessMidasEvent(TDataContainer& dataContainer);

  /// Update those histograms that only need to be updated when we are plotting.
  void UpdateForPlotting();

  void Initialize();

  bool CheckOption(std::string option);
    
  void BeginRun(int transition,int run,int time) {};
  void EndRun(int transition,int run,int time) {};

  // Add a THistogramArrayBase object to the list
  void AddHistogram(THistogramArrayBase* histo);

  // Little trick; we only fill the transient histograms here (not cumulative), since we don't want
  // to fill histograms for events that we are not displaying.
  // It is pretty slow to fill histograms for each event.
  void UpdateTransientPlots(TDataContainer& dataContainer);
  
  // Get the histograms
  std::vector<THistogramArrayBase*> GetHistograms() {
    return fHistos;
  }  

private:

  std::vector<THistogramArrayBase*> fHistos;

};



#endif


