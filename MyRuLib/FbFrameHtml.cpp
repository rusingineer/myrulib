#include "FbFrameHtml.h"
#include "FbDatabase.h"
#include "FbConst.h"
#include "FbParams.h"
#include "FbMainMenu.h"
#include "BaseThread.h"
#include "MyRuLibApp.h"
#include "InfoCash.h"
#include <wx/wfstream.h>
#include <wx/txtstrm.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>

BEGIN_EVENT_TABLE(FbFrameHtml, wxAuiMDIChildFrame)
    EVT_MENU(ID_HTML_SUBMIT, FbFrameHtml::OnSubmit)
    EVT_MENU(ID_HTML_MODIFY, FbFrameHtml::OnModify)
    EVT_MENU(ID_BOOKINFO_UPDATE, FbFrameHtml::OnInfoUpdate)
    EVT_MENU(wxID_SAVE, FbFrameHtml::OnSave)
    EVT_HTML_LINK_CLICKED(ID_HTML_DOCUMENT, FbFrameHtml::OnLinkClicked)
	EVT_TEXT_ENTER(ID_HTML_CAPTION, FbFrameHtml::OnSubmit)
END_EVENT_TABLE()

wxString FbFrameHtml::GetMd5sum(const int id)
{
	FbCommonDatabase database;
	wxString sql = wxT("SELECT md5sum FROM books WHERE id=?");
	wxSQLite3Statement stmt = database.PrepareStatement(sql);
	stmt.Bind(1, id);
	wxSQLite3ResultSet res = stmt.ExecuteQuery();
	if (res.NextRow())
		return res.GetString(0);
	else
		return wxEmptyString;
}

FbFrameHtml::FbFrameHtml(wxAuiMDIParentFrame * parent, BookTreeItemData & data)
    :wxAuiMDIChildFrame(parent, ID_FRAME_HTML, _("Комментарии")),
    m_id(data.GetId()), m_md5sum(GetMd5sum(m_id))
{
	CreateControls();
	InfoCash::UpdateInfo(this, m_id, false, true);
}

void FbFrameHtml::Load(const wxString & html)
{
	m_info.SetPage(html);
    m_info.SetFocus();
}

void FbFrameHtml::CreateControls()
{
	SetMenuBar(new FbMainMenu);

	wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	wxSplitterWindow * splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxSize(500, 400), wxSP_3D);
	splitter->SetMinimumPaneSize(80);
	splitter->SetSashGravity(1);
	sizer->Add(splitter, 1, wxEXPAND);

	m_info.Create(splitter, ID_HTML_DOCUMENT);

	wxPanel * panel = new wxPanel( splitter, wxID_ANY, wxDefaultPosition, wxSize(-1, 80), wxTAB_TRAVERSAL );
	wxBoxSizer * bSizerComment = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizerSubject;
	bSizerSubject = new wxBoxSizer( wxHORIZONTAL );

	wxStaticText * staticText = new wxStaticText( panel, wxID_ANY, wxT("Комментарий:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText->Wrap( -1 );
	bSizerSubject->Add( staticText, 0, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_Caption.Create( panel, ID_HTML_CAPTION, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
	bSizerSubject->Add( &m_Caption, 1, wxALL|wxALIGN_CENTER_VERTICAL, 5 );

	m_ToolBar.Create( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_FLAT|wxTB_HORIZONTAL|wxTB_NODIVIDER|wxTB_NOICONS|wxTB_TEXT );
	m_ToolBar.AddTool( ID_HTML_SUBMIT, wxT("Добавить"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_ToolBar.AddTool( ID_HTML_MODIFY, wxT("Изменить"), wxNullBitmap, wxNullBitmap, wxITEM_NORMAL, wxEmptyString, wxEmptyString );
	m_ToolBar.EnableTool(ID_HTML_MODIFY, false);
	m_ToolBar.Realize();

	bSizerSubject->Add( &m_ToolBar, 0, wxALIGN_CENTER_VERTICAL, 5 );

	bSizerComment->Add( bSizerSubject, 0, wxEXPAND, 5 );

	m_Comment.Create( panel, ID_HTML_COMMENT, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_WORDWRAP );
	bSizerComment->Add( &m_Comment, 1, wxEXPAND|wxBOTTOM|wxRIGHT|wxLEFT, 5 );

	panel->SetSizer( bSizerComment );
	panel->Layout();
	bSizerComment->Fit( panel );

	splitter->SplitHorizontally(&m_info, panel, GetClientRect().y - 150);

	SetSizer(sizer);
	Layout();
}

void FbFrameHtml::OnSave(wxCommandEvent& event)
{
    wxFileDialog dlg (
		this,
		_("Выберите файл для экспорта отчета"),
		wxEmptyString,
		wxT("lib_info.html"),
		_("Файы HTML (*.html; *.htm)|*.html;*.HTML;*.HTM;*.htm|Все файлы (*.*)|*.*"),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT
    );

	if (dlg.ShowModal() == wxID_OK) {
   		wxString html = * m_info.GetParser()->GetSource();
        wxFileOutputStream stream(dlg.GetPath());
        wxTextOutputStream text(stream);
		text.WriteString(html);
	}

}

void FbFrameHtml::OnInfoUpdate(wxCommandEvent& event)
{
	if (event.GetInt() == m_id) {
		wxString html = event.GetString();
		m_info.SetPage(html);
//		wxMessageBox(html);
	}
}

wxString FbFrameHtml::GetComments()
{
	wxString sql = wxT("SELECT id, posted, caption, comment FROM comments WHERE md5sum=? ORDER BY id");

	FbLocalDatabase database;
	wxSQLite3Statement stmt = database.PrepareStatement(sql);
	stmt.Bind(1, m_md5sum);
	wxSQLite3ResultSet res = stmt.ExecuteQuery();

	wxString html;
	html += wxT("<TR><TD valign=top>");
	html += wxT("<TABLE>");

	while (res.NextRow()) {
		int id = res.GetInt(0);
		wxString caption = FbBookThread::HTMLSpecialChars(res.GetString(2));
		wxString comment = FbBookThread::HTMLSpecialChars(res.GetString(3));
		int pos;
		while ( (pos = comment.Find(wxT('\n'))) != wxNOT_FOUND) {
			comment = comment.Left(pos) + wxT("<br>") + comment.Mid(pos + 1);
		}
		html += wxString::Format(wxT("<TR><TD><B>%s: %s&nbsp;<A href=%d>&lt;удалить&gt;</A></B></TD></TR>"), res.GetString(1).c_str(), caption.c_str(), id);
		html += wxString::Format(wxT("<TR><TD>%s</TD></TR>"), comment.c_str());
	}

	html += wxT("</TR></TD>");
	html += wxT("</TABLE></TABLE></BODY></HTML>");

	return html;
}

void FbFrameHtml::OnSubmit(wxCommandEvent& event)
{
	wxString caption = m_Caption.GetValue();
	wxString comment = m_Comment.GetValue();
	if ( caption.IsEmpty() && comment.IsEmpty() ) return;

	wxString sql = wxT("INSERT INTO comments(id, md5sum, posted, caption, comment) VALUES (?,?,?,?,?)");

	FbDatabase & database = wxGetApp().GetConfigDatabase();
	int key = database.NewId(FB_NEW_COMMENT);
	wxSQLite3Statement stmt = database.PrepareStatement(sql);
	stmt.Bind(1, key);
	stmt.Bind(2, m_md5sum);
	stmt.Bind(3, wxDateTime::Now().FormatISODate() + wxT(" ") + wxDateTime::Now().FormatISOTime());
	stmt.Bind(4, caption);
	stmt.Bind(5, comment);
	stmt.ExecuteUpdate();

	m_Caption.SetValue(wxEmptyString);
	m_Comment.SetValue(wxEmptyString);
	m_ToolBar.EnableTool(ID_HTML_MODIFY, false);

	m_key = 0;

	InfoCash::UpdateInfo(this, m_id, false, true);
}

void FbFrameHtml::OnModify(wxCommandEvent& event)
{
	wxString caption = m_Caption.GetValue();
	wxString comment = m_Comment.GetValue();
	if ( caption.IsEmpty() && comment.IsEmpty() ) return;

	wxString sql = wxT("UPDATE comments SET posted=?, caption=?, comment=? WHERE id=?");

	FbDatabase & database = wxGetApp().GetConfigDatabase();
	wxSQLite3Statement stmt = database.PrepareStatement(sql);
	stmt.Bind(1, wxDateTime::Now().FormatISODate() + wxT(" ") + wxDateTime::Now().FormatISOTime());
	stmt.Bind(2, caption);
	stmt.Bind(3, comment);
	stmt.Bind(4, m_key);
	stmt.ExecuteUpdate();

	m_Caption.SetValue(wxEmptyString);
	m_Comment.SetValue(wxEmptyString);
	m_ToolBar.EnableTool(ID_HTML_MODIFY, false);

	m_key = 0;

	InfoCash::UpdateInfo(this, m_id, false, true);
}

void FbFrameHtml::OnLinkClicked(wxHtmlLinkEvent& event)
{
	FbLocalDatabase database;
	wxString id = event.GetLinkInfo().GetHref();

	if ( event.GetLinkInfo().GetTarget() == wxT("D") )
	{
		int res = wxMessageBox(_("Удалить комментарий?"), _("Подтверждение"), wxOK|wxCANCEL);
		if (res != wxOK) return;

		wxString sql = wxT("DELETE FROM comments WHERE id=") + id;
		database.ExecuteUpdate(sql);
		InfoCash::UpdateInfo(this, m_id, false, true);
	}
	else if ( event.GetLinkInfo().GetTarget() == wxT("M") )
	{
		wxString sql = wxT("SELECT id, caption, comment FROM comments WHERE id=") + id;
		wxSQLite3ResultSet res = database.ExecuteQuery(sql);
		if (res.NextRow()) {
			m_key = res.GetInt(0);
			m_Caption.SetValue( res.GetString(1) );
			m_Comment.SetValue( res.GetString(2) );
			m_ToolBar.EnableTool(ID_HTML_MODIFY, true);
		}
	}
}