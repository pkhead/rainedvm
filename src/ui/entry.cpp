#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#   include <wx/wx.h>
#endif

#include <wx/wxhtml.h>

constexpr int ITEM_SPACING = 2;

enum {
    ID_VersionList = wxID_HIGHEST,
    ID_InstallButton,
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

class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame {
public:
    MyFrame();

private:
    wxButton *installButton;
    wxListBox *versionListBox;

    void OnVersionSelect(wxCommandEvent &event);
    void OnExit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnInstallButtonPressed(wxCommandEvent &event);
};

bool MyApp::OnInit() {
    SetAppearance(Appearance::System);

    MyFrame *frame = new MyFrame();
    frame->Show(true);
    return true;
}

MyFrame::MyFrame()
    : wxFrame(nullptr, wxID_ANY, "Rained Version Manager")
{
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");

    wxPanel *mainPanel = new wxPanel(this, wxID_ANY);

    // 1. current version
    // 2. sizer1 (horiz)
    wxBoxSizer *sizer0 = new wxBoxSizer(wxVERTICAL);

    // 1. list box
    // 2. sizer2 (vert)
    wxBoxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);

    // 1. version changelog
    // 2. install button
    wxBoxSizer *sizer2 = new wxBoxSizer(wxVERTICAL);

    sizer0->Add(
        new wxStaticText(mainPanel, -1, "Current version: v2.3.2-dev"),
        0,
        wxALL,
        ITEM_SPACING);
    sizer0->Add(sizer1, 1, wxALL | wxEXPAND, ITEM_SPACING);

    wxString items[4];
    items[0] = "Nightly (current)";
    items[1] = "v2.3.2";
    items[2] = "v2.3.1";
    items[3] = "v2.3.0";
    versionListBox = new wxListBox(mainPanel, ID_VersionList,
                                    wxDefaultPosition, wxDefaultSize);
    versionListBox->InsertItems(wxArrayString(4, items), 0);
    versionListBox->Select(0);

    sizer1->Add(versionListBox, 0, wxALL | wxEXPAND, ITEM_SPACING);
    sizer1->Add(
        sizer2,
        1,
        wxALL | wxEXPAND,
        ITEM_SPACING);
    
    OverriddenHtmlWindow *htmlContent = new OverriddenHtmlWindow(
        mainPanel, -1, wxDefaultPosition, wxDefaultSize);
    htmlContent->LoadPage("page.html");
    sizer2->Add(
        htmlContent,
        1, wxALL | wxEXPAND, ITEM_SPACING
    );

    sizer2->Add(
        (installButton = new wxButton(mainPanel, ID_InstallButton, "Install")),
        0, wxALL, ITEM_SPACING
    );

    mainPanel->SetSizerAndFit(sizer0);

    SetMenuBar(menuBar);

    Bind(wxEVT_LISTBOX, &MyFrame::OnVersionSelect, this, ID_VersionList);
    Bind(wxEVT_BUTTON, &MyFrame::OnInstallButtonPressed, this, ID_InstallButton);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
}

void MyFrame::OnExit(wxCommandEvent &event) {
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent &event) {
    wxMessageBox("This is a wxWidgets Hello World example",
        "About Hello World", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnVersionSelect(wxCommandEvent &event) {
    if (versionListBox->GetSelection() == 0) {
        installButton->SetLabel("Sync");
    } else {
        installButton->SetLabel("Install");
    }
}

void MyFrame::OnInstallButtonPressed(wxCommandEvent &event) {
    wxLogMessage("install button pressed");
}

wxIMPLEMENT_APP(MyApp);