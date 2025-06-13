// ────────────────────────────────────────────────────────────
//  src/AsymmetryPW.C   – side-band asymmetry fitter
// ----------------------------------------------------------------
#include <TFile.h>
#include <TTree.h>
#include <TMath.h>
#include <TSystem.h>

#include <RooRealVar.h>
#include <RooDataSet.h>
#include <RooGenericPdf.h>
#include <RooArgList.h>
#include <RooFitResult.h>
#include <RooFit.h>

#include <vector>
#include <tuple>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace RooFit;

// helper ──────────────────────────────────────────────────────
static std::string legendreString(int l,int m)
{
  if (std::abs(m)>l) return "0.0";
  if (l==0 && m==0)  return "1.0";
  if (l==1){
      if (m==0) return "cos(th)";
      if (std::abs(m)==1) return "sin(th)";
  }
  if (l==2){
      if (m==0) return "0.5*(3*cos(th)*cos(th)-1)";
      if (std::abs(m)==1) return "sin(2*th)";
      if (std::abs(m)==2) return "sin(th)*sin(th)";
  }
  return "0.0";
}

// container describing one amplitude term
struct TermDesc{
  int l,m,t;
  std::string name;        // e.g. b_0 …
  std::string modulation;  // P_lm * sin(...)
};

// ────────────────────────────────────────────────────────────
//  class
// ----------------------------------------------------------------
class AsymmetryPW{
public:
  AsymmetryPW(const char* root,const char* tree,
              const char* pair,const char* out);
  void Loop();

private:
  void buildTerms();
  std::string buildMod(const std::string& pref,
                       bool numeric=false,
                       const std::vector<double>& val={}) const;

  std::string rootFile_, treeName_, pair_, outDir_;
  std::vector<TermDesc> termList_;
  std::vector<std::string> purityBranches_;          // purity_* names
};

// ─── constructor ────────────────────────────────────────────
AsymmetryPW::AsymmetryPW(const char* r,const char* t,
                         const char* p,const char* o)
: rootFile_(r), treeName_(t), pair_(p), outDir_(o)
{
  TFile f(rootFile_.c_str(),"READ");
  if (!f.IsZombie()) {
     if (auto tr = dynamic_cast<TTree*>(f.Get(treeName_.c_str()))) {
        std::cout<<"purity branches in "<<treeName_<<":\n";
        TObjArray* bl = tr->GetListOfBranches();
        for (int i=0;i<bl->GetEntries();++i){
           const char* nm = bl->At(i)->GetName();
           if (strncmp(nm,"purity_",7)==0 &&
               strncmp(nm,"purity_err_",11)!=0){
               purityBranches_.push_back(nm);
               std::cout<<"  "<<nm<<"\n";
           }
        }
     }
  }
  f.Close();
  buildTerms();
}

// build twelve partial-wave terms
void AsymmetryPW::buildTerms()
{
  termList_.clear();
  // twist-2
  for(int l=1;l<=2;++l)
    for(int m=1;m<=l;++m){
        TermDesc d{l,m,2};
        d.name="b_"+std::to_string(termList_.size());
        std::ostringstream ss;
        ss<<"("<<legendreString(l,m)<<")*sin("<<m<<"*phi_h - "<<m<<"*phi_R1)";
        d.modulation=ss.str();
        termList_.push_back(d);
    }
  // twist-3
  for(int l=0;l<=2;++l)
    for(int m=-l;m<=l;++m){
        TermDesc d{l,m,3};
        d.name="b_"+std::to_string(termList_.size());
        std::ostringstream ss;
        ss<<"("<<legendreString(l,m)<<")*sin("<<(1-m)<<"*phi_h "
          << (m>=0?"+":"-") << std::abs(m)<<"*phi_R1)";
        d.modulation=ss.str();
        termList_.push_back(d);
    }
}

// build Σ ai*mod string
std::string AsymmetryPW::buildMod(const std::string& pref,
                                  bool numeric,
                                  const std::vector<double>& v) const
{
  std::ostringstream out;
  for(size_t i=0;i<termList_.size();++i){
      if(i) out<<" + ";
      out<<(numeric? std::to_string(v[i]) : (pref+termList_[i].name))
         <<" * "<<termList_[i].modulation;
  }
  return out.str();
}

// ─── main loop ───────────────────────────────────────────────
void AsymmetryPW::Loop()
{
  gSystem->mkdir(outDir_.c_str(),true);
  std::ofstream yaml(outDir_+"/asymmetry_results.yaml");
  yaml<<"results:\n";

  // open ROOT
  TFile f(rootFile_.c_str(),"READ");
  if (f.IsZombie()){ std::cerr<<"bad file\n"; return; }
  auto* tree = static_cast<TTree*>(f.Get(treeName_.c_str()));
  if (!tree){ std::cerr<<"tree missing\n"; return; }

  // observables
  RooRealVar phi_h("phi_h","phi_h",-TMath::Pi(),TMath::Pi());
  RooRealVar phi_R1("phi_R1","phi_R1",-TMath::Pi(),TMath::Pi());
  RooRealVar th("th","th",0,TMath::Pi());
  RooRealVar hel("hel","hel",-1,1);
  RooRealVar M2("M2","M2",0,10);      // wide range

  // create RooRealVars for every purity_N_M
  std::vector<RooRealVar*> purityVars;
  for(auto& nm:purityBranches_)
      purityVars.push_back(new RooRealVar(nm.c_str(),nm.c_str(),-10,10));

  RooArgSet obs(phi_h,phi_R1,th,hel,M2);
  for(auto* pv:purityVars) obs.add(*pv);

  RooDataSet full("full","full",tree,obs);

  bool pi0 = (pair_=="piplus_pi0"||pair_=="piminus_pi0");

  // prepare data sets
  RooDataSet* dBack = nullptr;
  RooDataSet* dSign = nullptr;
  if (pi0){
      dBack = static_cast<RooDataSet*>(full.reduce("M2>0.2 && M2<0.4"));
      dSign = static_cast<RooDataSet*>(full.reduce("M2>0.106 && M2<0.166"));
  }

  // ---- background fit -------------------------------------------------------
  std::vector<double> bkgVal(termList_.size(),0.);
  if (pi0){
      yaml<<"  - region: background\n";
      yaml<<"    entries: "<<dBack->numEntries()<<"\n";
      if (dBack->numEntries()==0){
          yaml<<"    fit_failed: true\n";
      } else {
          RooArgList p;  std::vector<RooRealVar*> pvec;
          for(auto& t:termList_){
              auto* v=new RooRealVar(("bkg_"+t.name).c_str(),
                                     ("bkg_"+t.name).c_str(),0.,-2.,2.);
              p.add(*v); pvec.push_back(v);
          }
          RooArgList pdfObs(phi_h,phi_R1,th,hel);
          RooArgList all(pdfObs); all.add(p);
          std::string expr="1 + hel * ("+buildMod("bkg_",false)+")";
          RooGenericPdf pdf("bkgpdf","bkgpdf",expr.c_str(),all);
          std::cout << "Fitting..." << std::endl;


          //////////////////////////////////////////////////////////////////////////
          /////////////               UN-BINNED FIT                    /////////////
          //////////////////////////////////////////////////////////////////////////
          //auto* res = pdf.fitTo(*dBack,Save(true),PrintLevel(1),EvalBackend("cpu"));
          RooFitResult* res = pdf.fitTo(*dBack,
              Save(true),
              NumCPU(0),
              PrintLevel(1),
              Optimize(2),
              Strategy(0),
              Minimizer("Minuit2","migrad")
          );
          //////////////////////////////////////////////////////////////////////////
          /////////////               BINNED FIT                    /////////////
          /////////////////////////////////////////////////////////////////////////
          // 1) build your binned histogram from the RooDataSet
          // RooDataHist dhBack("dhBack","dhBack", pdfObs, *dBack);
            
          // // 2) build a binned‐PDF out of it
          // RooHistPdf histPdfBack("histPdfBack","histPdfBack", pdfObs, dhBack);
            
          // // 3) fit the histPdfBack to dhBack (not the unbinned pdf to dBack)
          // auto* res = histPdfBack.fitTo(
          //     dhBack,
          //     Save(true),
          //     PrintLevel(1),
          //     NumCPU(0),      // spawn threads if you like
          //     Optimize(2)     // cache expressions
          // );





          
          if (res && res->status()==0 && res->covQual()>=2){
              for(size_t i=0;i<pvec.size();++i){
                 bkgVal[i]=pvec[i]->getVal();
                 yaml<<"    "<<pvec[i]->GetName()<<": "<<bkgVal[i]<<"\n";
                 yaml<<"    "<<pvec[i]->GetName()<<"_err: "<<pvec[i]->getError()<<"\n";
              }
          } else yaml<<"    fit_failed: true\n";
          delete res;  for(auto* v:pvec) delete v;
      }
  }

  // ---- signal fits for every purity branch ----------------------------------
  for(size_t ip=0; ip<purityBranches_.size(); ++ip)
  {
      const std::string& puName = purityBranches_[ip];
      RooDataSet* ds = pi0? dSign : &full;
      std::string region = "signal_"+puName;

      yaml<<"  - region: "<<region<<"\n";
      yaml<<"    entries: "<<ds->numEntries()<<"\n";

      if (ds->numEntries()==0){
          yaml<<"    fit_failed: true\n"; continue;
      }

      // free parameters
      RooArgList pars; std::vector<RooRealVar*> pvec;
      std::string pref="sig_"+puName+"_";
      for(auto& t:termList_){
          auto* v=new RooRealVar((pref+t.name).c_str(),
                                 (pref+t.name).c_str(),0.,-2.,2.);
          pars.add(*v); pvec.push_back(v);
      }

      // build modulation with this purity branch
      std::string sig  = buildMod(pref,false);
      std::string bkgF = buildMod("",true,bkgVal);
      std::string mod  = pi0 ?
           (puName+"*("+sig+") + (1-"+puName+")*("+bkgF+")") :
            sig;
      std::string expr = "1 + hel * ("+mod+")";

      RooArgList pdfObs(phi_h,phi_R1,th,hel);
      pdfObs.add(*purityVars[ip]);                 // this branch only
      RooArgList all(pdfObs); all.add(pars);
      RooGenericPdf pdf(("pdf_"+puName).c_str(),("pdf_"+puName).c_str(),
                        expr.c_str(),all);
      std::cout << "Fitting..." << std::endl;
      RooFitResult* res = pdf.fitTo(*ds,
          Save(true),
          NumCPU(0),
          PrintLevel(1),
          Optimize(2),
          Strategy(0),
          Minimizer("Minuit2","migrad")
      );
      if (res && res->status()==0 && res->covQual()>=2){
          for(size_t i=0;i<pvec.size();++i){
              yaml<<"    "<<pvec[i]->GetName()<<": "<<pvec[i]->getVal()<<"\n";
              yaml<<"    "<<pvec[i]->GetName()<<"_err: "<<pvec[i]->getError()<<"\n";
          }
      } else yaml<<"    fit_failed: true\n";
      delete res; for(auto* v:pvec) delete v;
  }

  // cleanup
  if (pi0){ delete dBack; delete dSign; }
  for(auto* pv:purityVars) delete pv;
  f.Close(); yaml.close();
  std::cout<<"Wrote "<<outDir_<<"/asymmetry_results.yaml\n";
}

// wrapper for Ruby module
void asymmetry(const char* input,
               const char* tree,
               const char* pair,
               const char* outDir)
{
  gSystem->mkdir(outDir,true);
  AsymmetryPW job(input,tree,pair,outDir);
  job.Loop();
}
