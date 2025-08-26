#include <vector>
#include <atomic>
#include <thread>

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#   include <wx/wx.h>
#endif
#include <wx/wxhtml.h>
#include <wx/progdlg.h>

#include "../vm/vm.hpp"
#include "../sys.hpp"
#include "../sys_args_internal.hpp"

constexpr int ITEM_SPACING = 2;

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

class MyApp : public wxApp {
public:
    virtual bool OnInit();
};

class MyFrame : public wxFrame {
public:
    MyFrame();

private:
    std::atomic_bool is_vm_fetch_done = false;
    VersionManager vm;
    int activeVersionIndex = -1;
    int selectedVersionIndex = -1;

    wxButton *installButton;
    wxListBox *versionListBox;
    wxStaticText *versionLabel;

    wxPanel *mainPanel = nullptr;
    wxTimer *timer = nullptr;
    std::unique_ptr<std::thread> thread;

    void ClearPage();
    void ConstructFetchPage();
    void ConstructVersionSelector();

    void OnVersionSelect(wxCommandEvent &event);
    void OnExit(wxCommandEvent &event);
    void OnAbout(wxCommandEvent &event);
    void OnInstallButtonPressed(wxCommandEvent &event);
    void OnFetchCheckTimer(wxTimerEvent &event);
};

bool MyApp::OnInit() {
    sys::set_arguments(argc, argv);
#if wxVERSION_NUMBER >= 3300
    SetAppearance(Appearance::System);
#endif

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
    SetMenuBar(menuBar);

    mainPanel = new wxPanel(this, wxID_ANY);
    ConstructFetchPage();

    timer = new wxTimer(this, ID_FetchCheckTimer);
    timer->Start(1);

    thread = std::make_unique<std::thread>(([this]{
        vm.fetch();
        wxMilliSleep(1000);
        is_vm_fetch_done = true;
    }));

    Bind(wxEVT_LISTBOX, &MyFrame::OnVersionSelect, this, ID_VersionList);
    Bind(wxEVT_BUTTON, &MyFrame::OnInstallButtonPressed, this, ID_InstallButton);
    Bind(wxEVT_TIMER, &MyFrame::OnFetchCheckTimer, this, ID_FetchCheckTimer);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
}

void MyFrame::ClearPage() {
    if (mainPanel) {
        mainPanel->DestroyChildren();
    }

    installButton = nullptr;
    versionListBox = nullptr;
    versionLabel = nullptr;
}

void MyFrame::ConstructFetchPage() {
    ClearPage();

    // mainPanel = new wxPanel(this);
    wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

    sizer->AddStretchSpacer(1);

    sizer->Add(
        new wxStaticText(mainPanel, wxID_ANY, "Fetching version list..."),
        0, wxALL | wxALIGN_CENTER, ITEM_SPACING
    );

    wxGauge *gauge = new wxGauge(mainPanel, wxID_ANY, 100);
    gauge->Pulse();

    sizer->Add(
        gauge,
        0, wxALL | wxALIGN_CENTER, ITEM_SPACING
    );

    sizer->AddStretchSpacer(1);

    mainPanel->SetSizerAndFit(sizer);
}

void MyFrame::ConstructVersionSelector() {
    ClearPage();

    // mainPanel = new wxPanel(this, wxID_ANY);

    // 1. current version
    // 2. sizer1 (horiz)
    wxBoxSizer *sizer0 = new wxBoxSizer(wxVERTICAL);

    // 1. list box
    // 2. sizer2 (vert)
    wxBoxSizer *sizer1 = new wxBoxSizer(wxHORIZONTAL);

    // 1. version changelog
    // 2. install button
    wxBoxSizer *sizer2 = new wxBoxSizer(wxVERTICAL);

    versionLabel =
        new wxStaticText(mainPanel, -1, "Current version: v2.3.2-dev");
    
    if (vm.get_current_version_name().empty())
        versionLabel->Hide();

    sizer0->Add(
        versionLabel,
        0,
        wxALL,
        ITEM_SPACING);
    sizer0->Add(sizer1, 1, wxALL | wxEXPAND, ITEM_SPACING);

    wxArrayString versionItems;
    activeVersionIndex = vm.get_current_version_index();
    selectedVersionIndex = activeVersionIndex;

    // compile list of version names, and also determine the index of the active
    // version to have automatically already selected
    const auto &available_versions = vm.get_available_versions();
    for (auto it = available_versions.begin(); it != available_versions.end(); ++it) {
        const auto &info = *it;
        int index = (int)std::distance(available_versions.begin(), it);

        std::string name = info.version_name;
        if (index == activeVersionIndex) {
            name += " (current)";
        }

        versionItems.push_back(name);
    }
    
    // construct the version list box
    versionListBox = new wxListBox(
        mainPanel, ID_VersionList, wxDefaultPosition, wxDefaultSize);
    versionListBox->InsertItems(versionItems, 0);
    if (activeVersionIndex != -1)
        versionListBox->Select(activeVersionIndex);

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
    Layout();

    // this is to update the button label correctly
    wxCommandEvent tmp = wxCommandEvent(wxEVT_LISTBOX);
    OnVersionSelect(tmp);
}

void MyFrame::OnFetchCheckTimer(wxTimerEvent &event) {
    (void)event;

    if (is_vm_fetch_done) {
        delete timer;
        timer = nullptr;

        thread->join();
        thread = nullptr;

        wxLogDebug("vm fetch done");
        ConstructVersionSelector();
    }
}

void MyFrame::OnExit(wxCommandEvent &event) {
    (void)event;
    Close(true);
}

void MyFrame::OnAbout(wxCommandEvent &event) {
    (void)event;

    wxMessageBox("This is a wxWidgets Hello World example",
        "About Hello World", wxOK | wxICON_INFORMATION);
}

void MyFrame::OnVersionSelect(wxCommandEvent &event) {
    (void)event;

    selectedVersionIndex = versionListBox->GetSelection();
    if (activeVersionIndex != -1 && selectedVersionIndex == activeVersionIndex) {
        installButton->SetLabel("Sync");
    } else {
        installButton->SetLabel("Install");
    }

    installButton->Enable(selectedVersionIndex != -1);
}

void MyFrame::OnInstallButtonPressed(wxCommandEvent &event) {
    (void)event;

    if (selectedVersionIndex == -1) return;

    wxProgressDialog dlg(
        "Installing...", "", 1000, this,
        wxPD_APP_MODAL | wxPD_CAN_ABORT);
    
    std::unique_ptr<InstallTask> task = 
        vm.start_installation(vm.get_available_versions()[selectedVersionIndex]);
    
    while (true) {
        std::string msg;
        float progress;

        if (!task->get_progress(msg, progress)) {
            std::string exceptionMsg;
            if (task->get_exception(exceptionMsg)) {
                exceptionMsg = "Exception occurred\n\n" + exceptionMsg;
                wxMessageBox(
                    exceptionMsg, wxMessageBoxCaptionStr,
                    wxOK | wxCENTER | wxICON_ERROR, this);
            }

            task = nullptr;
            break;
        }
        
        if (!dlg.Update((int)(progress * 999), msg)) {
            task = nullptr;
            break;
        }

        wxMilliSleep(10);
    }
}

wxIMPLEMENT_APP(MyApp);