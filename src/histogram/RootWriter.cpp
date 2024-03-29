/*!
 * \file RootWriter.cpp
 * \brief Implementation of RootWriter.
 * \author unknown
 * \copyright GNU Public License v. 3
 */

#include "RootWriter.h"

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>
#include <TH3.h>

#include "Histogram1D.h"
#include "Histogram2D.h"
#include "Histogram3D.h"

// ########################################################################

void RootWriter::Navigate(Named *named, TFile *file)
{
    // Ensure we are at the top.
    file->cd();
    auto path = named->GetPath();
    if ( path.empty() )
        return;
    auto title = path;
    if ( path.find_last_of( '/') < path.npos ){
        title = path.substr(path.find_last_of('/')+1);
    }
    if ( file->mkdir(path.c_str(), title.c_str(), true) == nullptr ){
        throw std::runtime_error("Error, could not create directory '"+path+"'.");
    }

    // Navigate to the folder and return
    file->cd(path.c_str());
}

// ########################################################################

void RootWriter::Write(Histograms& histograms, const char *filename,
                       const char *title, const char *options)
{
  TFile outfile(filename, options, title);

  Histograms::list1d_t list1d = histograms.GetAll1D();
  for(auto & it : list1d) {
      Navigate(it, &outfile);
      CreateTH1(it);
  }

  Histograms::list2d_t list2d = histograms.GetAll2D();
  for(auto & it : list2d) {
      Navigate(it, &outfile);
      CreateTH2(it);
  }

    Histograms::list3d_t list3d = histograms.GetAll3D();
    for(auto & it : list3d) {
        Navigate(it, &outfile);
        CreateTH3(it);
    }

  outfile.Write();
  outfile.Close();
}

// ########################################################################

TH1p RootWriter::CreateTH1(Histogram1Dp h)
{
  const Axis& xax = h->GetAxisX();
  const int channels = xax.GetBinCount();
  TH1* r = new TH1I( h->GetName().c_str(), h->GetTitle().c_str(),
                     channels, xax.GetLeft(), xax.GetRight() );

  TAxis* rxax = r->GetXaxis();
  rxax->SetTitle(xax.GetTitle().c_str());
  rxax->SetTitleSize(0.03);
  rxax->SetLabelSize(0.03);

  TAxis* ryax = r->GetYaxis();
#if ROOT1D_YTITLE
  char buf[8];
    std::snprintf(buf, 8, "%.2f", xax.GetBinWidth());
    std::string ytitle = "Counts/" + std::string(buf);
    ryax->SetTitle(ytitle.c_str());
    ryax->SetTitleSize(0.03);
#endif // ROOT1D_YTITLE
  ryax->SetLabelSize(0.03);

  for(int i=0; i<channels+2; ++i)
    r->SetBinContent(i, h->GetBinContent(i));
  r->SetEntries( h->GetEntries() );

  return r;
}

// ########################################################################

TH2* RootWriter::CreateTH2(Histogram2Dp h)
{
  const Axis& xax = h->GetAxisX();
  const Axis& yax = h->GetAxisY();
  const int xchannels = xax.GetBinCount();
  const int ychannels = yax.GetBinCount();
  TH2* mat = new TH2F( h->GetName().c_str(), h->GetTitle().c_str(),
                       xchannels, xax.GetLeft(), xax.GetRight(),
                       ychannels, yax.GetLeft(), yax.GetRight() );
  mat->SetOption( "colz" );
  mat->SetContour( 64 );

  TAxis* rxax = mat->GetXaxis();
  rxax->SetTitle(xax.GetTitle().c_str());
  rxax->SetTitleSize(0.03);
  rxax->SetLabelSize(0.03);

  TAxis* ryax = mat->GetYaxis();
  ryax->SetTitle(yax.GetTitle().c_str());
  ryax->SetTitleSize(0.03);
  ryax->SetLabelSize(0.03);
  ryax->SetTitleOffset(1.3);

  TAxis* zax = mat->GetZaxis();
  zax->SetLabelSize(0.025);

  for(int iy=0; iy<ychannels+2; ++iy)
    for(int ix=0; ix<xchannels+2; ++ix)
      mat->SetBinContent(ix, iy, h->GetBinContent(ix, iy));
  mat->SetEntries( h->GetEntries() );

  return mat;
}

// ########################################################################

TH3* RootWriter::CreateTH3(Histogram3Dp h)
{
    const Axis& xax = h->GetAxisX();
    const Axis& yax = h->GetAxisY();
    const Axis& zax = h->GetAxisZ();
    const auto xchannels = xax.GetBinCount();
    const auto ychannels = yax.GetBinCount();
    const auto zchannels = zax.GetBinCount();
    TH3* cube = new TH3F( h->GetName().c_str(), h->GetTitle().c_str(),
                          xchannels, xax.GetLeft(), xax.GetRight(),
                          ychannels, yax.GetLeft(), yax.GetRight(),
                         zchannels, zax.GetLeft(), zax.GetRight());
    cube->SetOption( "colz" );
    cube->SetContour( 64 );

    TAxis* rxax = cube->GetXaxis();
    rxax->SetTitle(xax.GetTitle().c_str());
    rxax->SetTitleSize(0.03);
    rxax->SetLabelSize(0.03);

    TAxis* ryax = cube->GetYaxis();
    ryax->SetTitle(yax.GetTitle().c_str());
    ryax->SetTitleSize(0.03);
    ryax->SetLabelSize(0.03);
    ryax->SetTitleOffset(1.3);

    TAxis* rzax = cube->GetZaxis();
    rzax->SetTitle(zax.GetTitle().c_str());
    rzax->SetLabelSize(0.025);

    for(Axis::index_t iz=0; iz<zchannels+2; ++iz)
        for(Axis::index_t iy=0; iy<ychannels+2; ++iy)
            for(Axis::index_t ix=0; ix<xchannels+2; ++ix)
                cube->SetBinContent(ix, iy, iz, h->GetBinContent(ix, iy, iz));
    cube->SetEntries( h->GetEntries() );

    return cube;
}

// ########################################################################
