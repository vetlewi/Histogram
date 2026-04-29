//
// Created by Vetle Wegner Ingeberg on 28/04/2026.
//


#include <iostream>
#include <TMemFile.h>
#include <TH2.h>
#include <THttpServer.h>

#include <histogram/SharedHistograms.h>
#include <csignal>

char leaveprog = 'n';

void keyb_int(int sig_num)
{
    if (sig_num == SIGINT || sig_num == SIGQUIT || sig_num == SIGTERM) {
        printf("\n\nLeaving...\n");
        leaveprog = 'y';
    }
}

TH1* CreateTH1(SharedHistogram1Dp h) {
    const Axis& xax = h->GetAxisX();
    const int channels = xax.GetBinCount();
    TH1* r = new TH1I( h->GetName().c_str(), h->GetTitle().c_str(),
                       channels, xax.GetLeft(), xax.GetRight() );

    TAxis* rxax = r->GetXaxis();
    rxax->SetTitle(xax.GetTitle().c_str());
    rxax->SetTitleSize(0.03);
    rxax->SetLabelSize(0.03);

    TAxis* ryax = r->GetYaxis();
    ryax->SetLabelSize(0.03);

    for(int i=0; i<channels+2; ++i)
        r->SetBinContent(i, h->GetBinContent(i));
    r->SetEntries( h->GetEntries() );

    return r;
}

TH2* CreateTH2(SharedHistogram2Dp h) {
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

void update_histogram(TH1* rhist, SharedHistogram1Dp oclhist) {

    // check that the root hist and the OCL hist are consistent.

    // Go bin-by-bin and update
    for (size_t bin = 0; bin < oclhist->GetAxisX().GetBinCount() + 2 ; ++bin) {
        rhist->SetBinContent(bin, oclhist->GetBinContent(bin));
    }
    rhist->SetEntries(oclhist->GetEntries());
}

void update_matrix(TH2* rmat, SharedHistogram2Dp oclmat) {
    for (size_t binX = 0; binX < oclmat->GetAxisX().GetBinCount() + 2 ; ++binX) {
        for (size_t binY = 0; binY < oclmat->GetAxisY().GetBinCount() + 2 ; ++binY) {
            rmat->SetBinContent(binX, binY, oclmat->GetBinContent(binX, binY));
        }
    }
    rmat->SetEntries(oclmat->GetEntries());
}

int main(int argc, char *argv[]) {

    signal(SIGINT, keyb_int); // set up interrupt handler (Ctrl-C)
    signal(SIGQUIT, keyb_int);
    signal(SIGTERM, keyb_int);
    signal(SIGPIPE, SIG_IGN);

    if ( argc != 1 ) {
        std::cerr << "Usage: " << argv[0] << std::endl;
    }

    TFile* file = new TMemFile("test.root", "RECREATE", "Demo");
    THttpServer* server = new THttpServer("http:8080");

    auto histograms = SharedHistograms::Attach("/test");
    auto h = histograms.Find1D("hist");
    auto m = histograms.Find2D("mat");

    TH1* hist = CreateTH1(h);
    TH2* mat = CreateTH2(m);

    server->SetTimer(0, true);
    server->Register("hist", hist);
    while ( leaveprog == 'n' ) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << h->GetEntries() << std::endl;
        update_histogram(hist, h);
        update_matrix(mat, m);
        server->ProcessRequests();
    }

    delete server;
    delete file;

}