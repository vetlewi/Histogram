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

void update_histogram(TH1* rhist, SharedHistogram1Dp oclhist) {

    // check that the root hist and the OCL hist are consistent.

    // Go bin-by-bin and update
    for (size_t bin = 0; bin < oclhist->GetAxisX().GetBinCount() + 2 ; ++bin) {
        rhist->SetBinContent(bin, oclhist->GetBinContent(bin));
    }
    rhist->SetEntries(oclhist->GetEntries());
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

    TH1* hist = CreateTH1(h);

    server->SetTimer(0, true);
    server->Register("hist", hist);
    while ( leaveprog == 'n' ) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << h->GetEntries() << std::endl;
        update_histogram(hist, h);
        server->ProcessRequests();
    }


    delete server;
    delete file;

}