#pragma once

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#   include <wx/wx.h>
#endif
#include <wx/wxhtml.h>

#include "../vm/vm.hpp"

constexpr int BORDER_WIDTH = 3;
constexpr int BORDER_WIDTH_LARGE = 6;

enum {
    ID_VersionList = wxID_HIGHEST,
    ID_InstallButton,

    ID_FetchCheckTimer,
};

class OverriddenHtmlWindow : public wxHtmlWindow {
public:
	OverriddenHtmlWindow(
        wxWindow *parent, wxWindowID id = -1,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxHW_SCROLLBAR_AUTO,
        const wxString& name = _T("htmlWindow"));
    
	void OnLinkClicked(const wxHtmlLinkInfo& link);
};

OverriddenHtmlWindow::OverriddenHtmlWindow(
    wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
    long style, const wxString& name
) : wxHtmlWindow(parent, id, pos, size, style, name) {}

void OverriddenHtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link) {
    wxLaunchDefaultBrowser(link.GetHref());
}

class RainedVMApp : public wxApp {
public:
    virtual bool OnInit();
};

class RainedVMFrame : public wxFrame {
public:
    RainedVMFrame();

private:
    std::atomic_bool is_vm_fetch_done = false;
    VersionManager vm;
    int activeVersionIndex = -1;
    int selectedVersionIndex = -1;

    wxButton *installButton;
    wxListBox *versionListBox;
    wxStaticText *versionLabel;
    wxHtmlWindow *htmlWindow;

    wxPanel *mainPanel = nullptr;
    wxTimer *timer = nullptr;
    std::unique_ptr<std::thread> thread;

    void Fetch();

    void ClearPage();
    void ConstructFetchPage();
    void ConstructVersionSelector();

    void OnVersionSelect(wxCommandEvent &event);
    void OnExit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnInstallButtonPressed(wxCommandEvent &event);
    void OnFetchCheckTimer(wxTimerEvent &event);
};

class AboutDialog : public wxDialog {
public:
    AboutDialog(wxWindow *parent, wxWindowID id);
};