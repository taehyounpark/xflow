#include <memory>
#include <chrono>
#include <iostream>
#include <sstream>

#include <ROOT/RVec.hxx>
#include "TPad.h"
#include "TFile.h"
#include "TLorentzVector.h"
#include "TVector2.h"
#include "TH1F.h"

#include "ana/analysis.h"
#include "ana/definition.h"
#include "ana/output.h"

#include "RAnalysis/TreeData.h"
#include "RAnalysis/Histogram.h"
#include "RAnalysis/Folder.h"

class ScaledP4 : public ana::column::definition<TLorentzVector(ROOT::RVec<double>, ROOT::RVec<double>, ROOT::RVec<double>, ROOT::RVec<double>)>
{

public:

  ScaledP4(unsigned int index, double scale=1.0) : 
    ana::column::definition<TLorentzVector(ROOT::RVec<double>, ROOT::RVec<double>, ROOT::RVec<double>, ROOT::RVec<double>)>(),
    m_index(index),
    m_scale(scale)
  {}

  virtual ~ScaledP4() = default;

  virtual TLorentzVector evaluate(ana::observable<ROOT::RVec<double>> pt, ana::observable<ROOT::RVec<double>> eta, ana::observable<ROOT::RVec<double>> phi, ana::observable<ROOT::RVec<double>> es) const override {
    TLorentzVector p4;
    p4.SetPtEtaPhiE(pt->at(m_index)*m_scale,eta->at(m_index),phi->at(m_index),es->at(m_index)*m_scale);
    return p4;
  }

protected:

  unsigned int m_index;
  double m_scale;

};
 
int main(int argc, char* argv[]) {

  ana::multithread::disable();
  ana::analysis<TreeData> hww;
  hww.open( "mini", {"hww_mc.root"} );

  auto mc_weight = hww.read<float>("mcWeight");
  auto el_sf = hww.read<float>("scaleFactor_ELE");
  auto mu_sf = hww.read<float>("scaleFactor_MUON");

  auto n_lep = hww.read<unsigned int>("lep_n");
  auto lep_pt_MeV = hww.read<ROOT::RVec<float>>("lep_pt").vary("lptcone30", "lep_ptcone30");
  auto lep_eta = hww.read<ROOT::RVec<float>>("lep_eta");
  auto lep_phi = hww.read<ROOT::RVec<float>>("lep_phi");
  auto lep_E_MeV = hww.read<ROOT::RVec<float>>("lep_E");
  auto lep_Q = hww.read<ROOT::RVec<float>>("lep_charge");
  auto lep_type = hww.read<ROOT::RVec<unsigned int>>("lep_type");
  auto met_MeV = hww.read<float>("met_et");
  auto met_phi = hww.read<float>("met_phi");

  auto GeV = hww.constant<double>(1000.0);
  auto lep_pt = lep_pt_MeV / GeV;  // RVec<float> / double
  auto lep_E = lep_E_MeV / GeV;
  auto met = met_MeV / GeV;

  auto eta_max = hww.constant<double>(2.4);

  auto l1p4 = hww.define<ScaledP4>(0)\
                  .vary("lp4_up",0,1.1)\
                  .vary("lp4_dn",0,0.9)\
                  (lep_pt, lep_eta, lep_phi, lep_E);

  auto l2p4 = hww.define<ScaledP4>(1)\
                  .vary("lp4_up",1,1.02)\
                  .vary("lp4_dn",1,0.98)\
                  (lep_pt, lep_eta, lep_phi, lep_E);

  auto llp4 = hww.define([](const TLorentzVector& p4, const TLorentzVector& q4){return (p4+q4);})(l1p4,l2p4);
  auto pth = hww.define(
    [](const TLorentzVector& llp4, float met, float met_phi) {
      TVector2 ptll; ptll.SetMagPhi(llp4.Pt(), llp4.Phi());
      TVector2 met2d; met2d.SetMagPhi(met, met_phi);
      return (ptll+met2d).Mod();
    })(llp4, met, met_phi);

  using cut = ana::selection::cut;
  using weight = ana::selection::weight;
  auto n_lep_req = hww.constant<int>(2);
  auto cut2l = hww.filter<weight>("mc_weight")(mc_weight * el_sf * mu_sf)\
                   .filter<cut>("2l")(n_lep == n_lep_req);
                    // reminder: delayed math operations

                                             // custom expressions also possible
  auto cut2los = cut2l.channel<cut>("2los",  [](const ROOT::RVec<float>& lep_charge){return lep_charge.at(0)+lep_charge.at(1)==0;})(lep_Q);
  auto cut2ldf = cut2los.filter<cut>("2ldf", [](const ROOT::RVec<int>& lep_type){return lep_type.at(0)+lep_type.at(1)==24;})(lep_type);
  auto cut2lsf = cut2los.filter<cut>("2lsf", [](const ROOT::RVec<int>& lep_type){return (lep_type.at(0)+lep_type.at(1)==22)||(lep_type.at(0)+lep_type.at(1)==26);})(lep_type);

  auto pth_2los = hww.book<Histogram<1,float>>(std::string("pth"),50,0,400).fill(pth).at(cut2los);

  auto pth_hists = hww.book<Histogram<1,float>>(std::string("pth"),50,0,400).fill(pth).at(cut2lsf,cut2ldf);

  auto get_pt = hww.define([](TLorentzVector const& p4){return p4.Pt();});
  auto l1pt = get_pt(l1p4);
  auto l2pt = get_pt(l2p4);
  auto l1n2pt_vars = hww.book<Histogram<1,float>>(std::string("l1n2pt"),50,0,200).fill(l1pt).fill(l2pt).at(cut2lsf, cut2ldf);

  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

  // auto pth_2los_res = pth_2los.result();  // std::shared_ptr<TH1F>

  // auto pth_2lsf_res = pth_hists["2los/2lsf"].result();
  // auto pth_2ldf_res = pth_hists["2los/2ldf"].result();

  auto l1n2pt_nom = l1n2pt_vars.nominal()["2los/2ldf"].result();
  auto l1n2pt_p4_up = l1n2pt_vars["lp4_up"]["2los/2ldf"].result();

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::cout << "Elapsed time = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;

  // gPad->Print("pth_channeled.pdf");
  // helper function to dump counter results at all selections
  auto out_file = TFile::Open("hww_results.root","recreate");
  ana::output::dump<Folder>(pth_hists, *out_file, "hww");
  ana::output::dump<Folder>(l1n2pt_vars, *out_file, "hww");
  delete out_file;

  l1n2pt_nom->SetLineColor(kBlack); l1n2pt_nom->Draw("hist");
  l1n2pt_p4_up->SetLineColor(kRed); l1n2pt_p4_up->Draw("E same");
  gPad->Print("l1n2pt_varied.pdf");

  return 0;
}