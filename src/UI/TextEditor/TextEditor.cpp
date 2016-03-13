
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextEditor.cpp
 * Description: The SLADE Text Editor control, does syntax
 *              highlighting, calltips, autocomplete and more,
 *              using an associated TextLanguage
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "TextEditor.h"
#include "Graphics/Icons.h"
#include "General/KeyBind.h"
#include <wx/app.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/gbsizer.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Int, txed_tab_width, 4, CVAR_SAVE)
CVAR(Bool, txed_auto_indent, true, CVAR_SAVE)
CVAR(Bool, txed_syntax_hilight, true, CVAR_SAVE)
CVAR(Bool, txed_brace_match, false, CVAR_SAVE)
CVAR(Int, txed_edge_column, 80, CVAR_SAVE)
CVAR(Bool, txed_indent_guides, false, CVAR_SAVE)
CVAR(String, txed_style_set, "SLADE Default", CVAR_SAVE)
CVAR(Bool, txed_calltips_mouse, true, CVAR_SAVE)
CVAR(Bool, txed_calltips_parenthesis, true, CVAR_SAVE)
CVAR(Bool, txed_fold_enable, true, CVAR_SAVE)
CVAR(Bool, txed_fold_comments, false, CVAR_SAVE)
CVAR(Bool, txed_fold_preprocessor, true, CVAR_SAVE)
CVAR(Bool, txed_trim_whitespace, false, CVAR_SAVE)
CVAR(Bool, txed_word_wrap, false, CVAR_SAVE)


/*******************************************************************
 * FINDREPLACEPANEL CLASS FUNCTIONS
 *******************************************************************/

/* FindReplacePanel::FindReplacePanel
 * FindReplacePanel class constructor
 *******************************************************************/
FindReplacePanel::FindReplacePanel(wxWindow* parent, TextEditor* text_editor)
	: wxPanel(parent, -1), text_editor(text_editor)
{
	wxGridBagSizer* gb_sizer = new wxGridBagSizer(4, 4);
	SetSizer(gb_sizer);

	// Find
	text_find = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	btn_find_next = new wxButton(this, -1, "Find Next");
	btn_find_prev = new wxButton(this, -1, "Find Previous");
	gb_sizer->Add(new wxStaticText(this, -1, "Find What:"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_find, wxGBPosition(0, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL|wxEXPAND);
	gb_sizer->Add(btn_find_next, wxGBPosition(0, 2), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(btn_find_prev, wxGBPosition(0, 3), wxDefaultSpan, wxEXPAND);

	// Replace
	text_replace = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	btn_replace = new wxButton(this, -1, "Replace");
	btn_replace_all = new wxButton(this, -1, "Replace All");
	gb_sizer->Add(new wxStaticText(this, -1, "Replace With:"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(text_replace, wxGBPosition(1, 1), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxEXPAND);
	gb_sizer->Add(btn_replace, wxGBPosition(1, 2), wxDefaultSpan, wxEXPAND);
	gb_sizer->Add(btn_replace_all, wxGBPosition(1, 3), wxDefaultSpan, wxEXPAND);

	// Options
	cb_match_case = new wxCheckBox(this, -1, "Match Case");
	cb_match_word_whole = new wxCheckBox(this, -1, "Match Word (Whole)");
	cb_match_word_start = new wxCheckBox(this, -1, "Match Word (Start)");
	cb_search_regex = new wxCheckBox(this, -1, "Regular Expression");
	gb_sizer->Add(cb_match_case, wxGBPosition(0, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	gb_sizer->Add(cb_search_regex, wxGBPosition(0, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	gb_sizer->Add(cb_match_word_whole, wxGBPosition(1, 4), wxDefaultSpan, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
	gb_sizer->Add(cb_match_word_start, wxGBPosition(1, 5), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);

	gb_sizer->AddGrowableCol(1);

	// Bind events
	btn_find_next->Bind(wxEVT_BUTTON, &FindReplacePanel::onBtnFindNext, this);
	btn_find_prev->Bind(wxEVT_BUTTON, &FindReplacePanel::onBtnFindPrev, this);
	btn_replace->Bind(wxEVT_BUTTON, &FindReplacePanel::onBtnReplace, this);
	btn_replace_all->Bind(wxEVT_BUTTON, &FindReplacePanel::onBtnReplaceAll, this);
	text_find->Bind(wxEVT_TEXT_ENTER, &FindReplacePanel::onTextFindEnter, this);
	text_replace->Bind(wxEVT_TEXT_ENTER, &FindReplacePanel::onTextReplaceEnter, this);
	Bind(wxEVT_CHAR_HOOK, &FindReplacePanel::onKeyDown, this);

	// Set tab order
	text_replace->MoveAfterInTabOrder(text_find);
}

/* FindReplacePanel::~FindReplacePanel
 * FindReplacePanel class destructor
 *******************************************************************/
FindReplacePanel::~FindReplacePanel()
{
}

/* FindReplacePanel::setFindText
 * Sets the 'Find' text to [find], selects all and focuses the text
 * box
 *******************************************************************/
void FindReplacePanel::setFindText(string find)
{
	text_find->SetFocus();
	text_find->SetValue(find);
	text_find->SelectAll();
}

/* FindReplacePanel::getFindText
 * Returns the current 'Find' text
 *******************************************************************/
string FindReplacePanel::getFindText()
{
	return text_find->GetValue();
}

/* FindReplacePanel::getFindFlags
 * Returns the selected search options
 *******************************************************************/
int FindReplacePanel::getFindFlags()
{
	int flags = 0;
	if (cb_match_case->GetValue())
		flags |= wxSTC_FIND_MATCHCASE;
	if (cb_match_word_start->GetValue())
		flags |= wxSTC_FIND_WORDSTART;
	if (cb_match_word_whole->GetValue())
		flags |= wxSTC_FIND_WHOLEWORD;
	if (cb_search_regex->GetValue())
		flags |= wxSTC_FIND_REGEXP;

	return flags;
}

/* FindReplacePanel::getReplaceText
 * Returns the current 'Replace' text
 *******************************************************************/
string FindReplacePanel::getReplaceText()
{
	return text_replace->GetValue();
}


/*******************************************************************
 * FINDREPLACEPANEL CLASS EVENTS
 *******************************************************************/

/* FindReplacePanel::onBtnFindNext
 * Called when the 'Find Next' button is clicked
 *******************************************************************/
void FindReplacePanel::onBtnFindNext(wxCommandEvent& e)
{
	text_editor->findNext(getFindText(), getFindFlags());
}

/* FindReplacePanel::onBtnFindPrev
 * Called when the 'Find Previous' button is clicked
 *******************************************************************/
void FindReplacePanel::onBtnFindPrev(wxCommandEvent& e)
{
	text_editor->findPrev(getFindText(), getFindFlags());
}

/* FindReplacePanel::onBtnReplace
 * Called when the 'Replace' button is clicked
 *******************************************************************/
void FindReplacePanel::onBtnReplace(wxCommandEvent& e)
{
	text_editor->replaceCurrent(getFindText(), getReplaceText(), getFindFlags());
}

/* FindReplacePanel::onBtnReplaceAll
 * Called when the 'Replace All' button is clicked
 *******************************************************************/
void FindReplacePanel::onBtnReplaceAll(wxCommandEvent& e)
{
	text_editor->replaceAll(getFindText(), getReplaceText(), getFindFlags());
}

/* FindReplacePanel::onKeyDown
 * Called when a key is pressed while the panel has focus
 *******************************************************************/
void FindReplacePanel::onKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	wxArrayString binds = KeyBind::getBinds(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	bool handled = false;
	for (unsigned a = 0; a < binds.size(); a++)
	{
		string name = binds[a];

		// Find next
		if (name == "ted_findnext")
		{
			text_editor->findNext(getFindText(), getFindFlags());
			handled = true;
		}

		// Find previous
		else if (name == "ted_findprev")
		{
			text_editor->findPrev(getFindText(), getFindFlags());
			handled = true;
		}

		// Replace next
		else if (name == "ted_replacenext")
		{
			text_editor->replaceCurrent(getFindText(), getReplaceText(), getFindFlags());
			handled = true;
		}

		// Replace all
		else if (name == "ted_replaceall")
		{
			text_editor->replaceAll(getFindText(), getReplaceText(), getFindFlags());
			handled = true;
		}
	}

	if (!handled)
	{
		// Esc = close panel
		if (e.GetKeyCode() == WXK_ESCAPE)
			text_editor->showFindReplacePanel(false);
		else
			e.Skip();
	}
}

/* FindReplacePanel::onTextFindEnter
 * Called when the enter key is pressed within the 'Find' text box
 *******************************************************************/
void FindReplacePanel::onTextFindEnter(wxCommandEvent& e)
{
	if (wxGetKeyState(WXK_SHIFT))
		text_editor->findPrev(getFindText(), getFindFlags());
	else
		text_editor->findNext(getFindText(), getFindFlags());
}

/* FindReplacePanel::onTextReplaceEnter
 * Called when the enter key is pressed within the 'Find' text box
 *******************************************************************/
void FindReplacePanel::onTextReplaceEnter(wxCommandEvent& e)
{
	text_editor->replaceCurrent(getFindText(), getReplaceText(), getFindFlags());
}


/*******************************************************************
 * TEXTEDITOR CLASS FUNCTIONS
 *******************************************************************/

/* TextEditor::TextEditor
 * TextEditor class constructor
 *******************************************************************/
TextEditor::TextEditor(wxWindow* parent, int id)
	: wxStyledTextCtrl(parent, id)
{
	// Init variables
	language = NULL;
	ct_argset = 0;
	ct_function = NULL;
	ct_start = 0;
	bm_cursor_last_pos = -1;
	panel_fr = NULL;

	// Set tab width
	SetTabWidth(txed_tab_width);

	// Line numbers by default
	SetMarginType(0, wxSTC_MARGIN_NUMBER);
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, "9999"));

	// Folding margin
	setupFoldMargin();

	// Border margin
	SetMarginWidth(2, 4);

	// Register icons for autocompletion list
	RegisterImage(1, Icons::getIcon(Icons::TEXT_EDITOR, "key"));
	RegisterImage(2, Icons::getIcon(Icons::TEXT_EDITOR, "const"));
	RegisterImage(3, Icons::getIcon(Icons::TEXT_EDITOR, "func"));

	// Init w/no language
	setLanguage(NULL);

	// Setup various configurable properties
	setup();

	// Bind events
	Bind(wxEVT_KEY_DOWN, &TextEditor::onKeyDown, this);
	Bind(wxEVT_KEY_UP, &TextEditor::onKeyUp, this);
	Bind(wxEVT_STC_CHARADDED, &TextEditor::onCharAdded, this);
	Bind(wxEVT_STC_UPDATEUI, &TextEditor::onUpdateUI, this);
	Bind(wxEVT_STC_CALLTIP_CLICK, &TextEditor::onCalltipClicked, this);
	Bind(wxEVT_STC_DWELLSTART, &TextEditor::onMouseDwellStart, this);
	Bind(wxEVT_STC_DWELLEND, &TextEditor::onMouseDwellEnd, this);
	Bind(wxEVT_LEFT_DOWN, &TextEditor::onMouseDown, this);
	Bind(wxEVT_KILL_FOCUS, &TextEditor::onFocusLoss, this);
	Bind(wxEVT_ACTIVATE, &TextEditor::onActivate, this);
	Bind(wxEVT_STC_MARGINCLICK, &TextEditor::onMarginClick, this);
}

/* TextEditor::~TextEditor
 * TextEditor class destructor
 *******************************************************************/
TextEditor::~TextEditor()
{
}

/* TextEditor::setup
 * Sets up text editor properties depending on cvars and the current
 * text styleset/style
 *******************************************************************/
void TextEditor::setup()
{
	// General settings
	SetBufferedDraw(true);
	SetUseAntiAliasing(true);
	SetMouseDwellTime(500);
	AutoCompSetIgnoreCase(true);
	SetIndentationGuides(txed_indent_guides);

	// Right margin line
	SetEdgeColumn(txed_edge_column);
	if (txed_edge_column == 0)
		SetEdgeMode(wxSTC_EDGE_NONE);
	else
		SetEdgeMode(wxSTC_EDGE_LINE);

	// Apply default style
	StyleSet::applyCurrent(this);
	CallTipUseStyle(10);
	StyleSetChangeable(wxSTC_STYLE_CALLTIP, true);
	wxFont font_ct(10, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	StyleSetFont(wxSTC_STYLE_CALLTIP, font_ct);
	CallTipSetForegroundHighlight(WXCOL(StyleSet::currentSet()->getStyle("calltip_hl")->getForeground()));

	// Set lexer
	if (txed_syntax_hilight)
		SetLexer(wxSTC_LEX_CPPNOCASE);
	else
		SetLexer(wxSTC_LEX_NULL);

	// Set folding options
	setupFolding();

	// Re-colour text
	Colourise(0, GetTextLength());

	// Set word wrapping
	if (txed_word_wrap)
		SetWrapMode(wxSTC_WRAP_WORD);
	else
		SetWrapMode(wxSTC_WRAP_NONE);
}

/* TextEditor::setupFoldMargin
 * Sets up the code folding margin
 *******************************************************************/
void TextEditor::setupFoldMargin(TextStyle* margin_style)
{
	if (!txed_fold_enable)
	{
		SetMarginWidth(1, 0);
		return;
	}

	wxColour col_fg, col_bg;
	if (margin_style)
	{
		col_fg = WXCOL(margin_style->getForeground());
		col_bg = WXCOL(margin_style->getBackground());
	}
	else
	{
		col_fg = WXCOL(StyleSet::currentSet()->getStyle("foldmargin")->getForeground());
		col_bg = WXCOL(StyleSet::currentSet()->getStyle("foldmargin")->getBackground());
	}

	SetMarginType(1, wxSTC_MARGIN_SYMBOL);
	SetMarginWidth(1, 16);
	SetMarginSensitive(1, true);
	SetMarginMask(1, wxSTC_MASK_FOLDERS);
	SetFoldMarginColour(true, col_bg);
	SetFoldMarginHiColour(true, col_bg);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_VLINE, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_LCORNER, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUSCONNECTED, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED, col_bg, col_fg);
	MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER, col_bg, col_fg);
}

/* TextEditor::setLanguage
 * Sets the text editor language
 *******************************************************************/
bool TextEditor::setLanguage(TextLanguage* lang)
{
	// Check language was given
	if (!lang)
	{
		// Clear keywords
		SetKeyWords(0, "");
		SetKeyWords(1, "");
		SetKeyWords(2, "");
		SetKeyWords(3, "");

		// Clear autocompletion list
		autocomp_list.Clear();
	}

	// Setup syntax hilighting if needed
	else
	{
		// Load word lists
		SetKeyWords(0, lang->getKeywordsList().Lower());
		SetKeyWords(1, lang->getFunctionsList().Lower());
		SetKeyWords(2, lang->getConstantsList().Lower());
		SetKeyWords(3, lang->getConstantsList().Lower());

		// Load autocompletion list
		autocomp_list = lang->getAutocompletionList();
	}

	// Set lexer
	if (txed_syntax_hilight)
		SetLexer(wxSTC_LEX_CPPNOCASE);
	else
		SetLexer(wxSTC_LEX_NULL);

	// Set folding options
	setupFolding();

	// Update variables
	SetWordChars("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.$");
	this->language = lang;

	// Re-colour text
	Colourise(0, GetTextLength());

	return true;
}

/* TextEditor::applyStyleSet
 * Applies the styleset [style] to the text editor
 *******************************************************************/
bool TextEditor::applyStyleSet(StyleSet* style)
{
	// Check if one was given
	if (!style)
		return false;

	// Apply it
	style->applyTo(this);

	return true;
}

/* TextEditor::loadEntry
 * Reads the contents of [entry] into the text area, returns false
 * if the given entry is invalid
 *******************************************************************/
bool TextEditor::loadEntry(ArchiveEntry* entry)
{
	// Clear current text
	ClearAll();

	// Check that the entry exists
	if (!entry)
	{
		Global::error = "Invalid archive entry given";
		return false;
	}

	// Check that the entry has any data, if not do nothing
	if (entry->getSize() == 0 || !entry->getData())
		return true;

	// Get character entry data
	//string text = wxString::From8BitData((const char*)entry->getData(), entry->getSize());
	string text = wxString::FromUTF8((const char*)entry->getData(), entry->getSize());
	// If opening as UTF8 failed for some reason, try again as 8-bit data
	if (text.length() == 0)
		text = wxString::From8BitData((const char*)entry->getData(), entry->getSize());

	// Load text into editor
	SetText(text);

	// Update line numbers margin width
	string numlines = S_FMT("0%d", GetNumberOfLines());
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, numlines));

	return true;
}

/* TextEditor::getRawText
 * Writes the raw ASCII text to [mc]
 *******************************************************************/
void TextEditor::getRawText(MemChunk& mc)
{
	mc.clear();
	string text = GetText();
	bool result = mc.importMem((const uint8_t*)text.ToUTF8().data(), text.ToUTF8().length());
}

/* TextEditor::trimWhitespace
 * Removes any unneeded whitespace from the ends of lines
 *******************************************************************/
void TextEditor::trimWhitespace()
{
	// Go through lines
	for (int a = 0; a < GetLineCount(); a++)
	{
		// Get line start and end positions
		int pos = GetLineEndPosition(a) - 1;
		int start = pos - GetLineLength(a);

		while (pos > start)
		{
			int chr = GetCharAt(pos);

			// Check for whitespace character
			if (chr == ' ' || chr == '\t')
			{
				// Remove character if whitespace
				Remove(pos, pos+1);
				pos--;
			}
			else
				break;	// Not whitespace, stop
		}
	}
}

/* TextEditor::showFindReplacePanel
 * Shows or hides the Find+Replace panel, depending on [show]. If
 * shown, fills the find text box with the current selection or the
 * current word at the caret
 *******************************************************************/
void TextEditor::showFindReplacePanel(bool show)
{
	// Do nothing if no F+R panel has been set
	if (!panel_fr)
		return;

	// Hide if needed
	if (!show)
	{
		panel_fr->Hide();
		panel_fr->GetParent()->Layout();
		SetFocus();
		return;
	}

	// Get currently selected text
	string find = GetSelectedText();

	// Get the word at the current cursor position if there is no current selection
	if (find.IsEmpty())
	{
		int ws = WordStartPosition(GetCurrentPos(), true);
		int we = WordEndPosition(GetCurrentPos(), true);
		find = GetTextRange(ws, we);
	}

	// Show the F+R panel
	panel_fr->Show();
	panel_fr->GetParent()->Layout();
	panel_fr->setFindText(find);
}

/* TextEditor::findNext
 * Finds the next occurrence of the [find] after the caret position,
 * selects it and scrolls to it if needed. Returns false if the
 * [find] was invalid or no match was found, true otherwise
 *******************************************************************/
bool TextEditor::findNext(string find, int flags)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Get current selection
	int sel_start = GetSelectionStart();
	int sel_end = GetSelectionEnd();

	// Search forwards from the end of the current selection
	SetSelection(GetCurrentPos(), GetCurrentPos());
	SearchAnchor();
	int found = SearchNext(flags, find);
	if (found < 0)
	{
		// Not found, loop back to start
		SetSelection(0, 0);
		SearchAnchor();
		found = SearchNext(flags, find);
		if (found < 0)
		{
			// No match found in entire text, reset selection
			SetSelection(sel_start, sel_end);
			return false;
		}
	}

	// Set caret to the end of the matching text
	// (it defaults to the start for some dumb reason)
	// and scroll to the selection
	SetSelection(found, found + find.length());
	EnsureCaretVisible();

	return true;
}

/* TextEditor::findPrev
 * Finds the previous occurrence of the [find] after the caret
 * position, selects it and scrolls to it if needed. Returns false
 * if the [find] was invalid or no match was found, true otherwise
 *******************************************************************/
bool TextEditor::findPrev(string find, int flags)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Get current selection
	int sel_start = GetSelectionStart();
	int sel_end = GetSelectionEnd();

	// Search back from the start of the current selection
	SetSelection(sel_start, sel_start);
	SearchAnchor();
	int found = SearchPrev(flags, find);
	if (found < 0)
	{
		// Not found, loop back to end
		SetSelection(GetTextLength() - 1, GetTextLength() - 1);
		SearchAnchor();
		found = SearchPrev(flags, find);
		if (found < 0)
		{
			// No match found in entire text, reset selection
			SetSelection(sel_start, sel_end);
			return false;
		}
	}

	// Set caret to the end of the matching text
	// (it defaults to the start for some dumb reason)
	// and scroll to the selection
	SetSelection(found, found + find.length());
	EnsureCaretVisible();

	return true;
}

/* TextEditor::replaceCurrent
 * Replaces the currently selected occurrence of [find] with
 * [replace], then selects and scrolls to the next occurrence of
 * [find] in the text. Returns false if [find] is invalid or the
 * current selection does not match it, true otherwise
 *******************************************************************/
bool TextEditor::replaceCurrent(string find, string replace, int flags)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Check that we've done a find previously
	// (by searching for the find string within the current selection)
	if (GetSelectedText().Length() != find.Length())
		return false;
	SetTargetStart(GetSelectionStart());
	SetTargetEnd(GetSelectionEnd());
	if (SearchInTarget(find) < 0)
		return false;

	// Do the replace
	ReplaceTarget(replace);

	// Update selection
	SetSelection(GetTargetStart(), GetTargetEnd());

	// Do find next
	findNext(find, flags);

	return true;
}

/* TextEditor::replaceAll
 * Replaces all occurrences of [find] in the text with [replace].
 * Returns the number of occurrences replaced
 *******************************************************************/
int TextEditor::replaceAll(string find, string replace, int flags)
{
	// Check search string
	if (find.IsEmpty())
		return false;

	// Start at beginning
	SetSelection(0, 0);

	// Loop of death
	int replaced = 0;
	while (true)
	{
		SearchAnchor();
		int found = SearchNext(flags, find);
		if (found < 0)
			break;	// No matches, finished
		else
		{
			// Replace text & increment counter
			Replace(found, found + find.length(), replace);
			replaced++;

			// Continue from end of replaced text
			SetSelection(found + find.length(), found + find.length());
		}
	}

	// Return number of instances replaced
	return replaced;
}

/* TextEditor::checkBraceMatch
 * Checks for a brace match at the current cursor position
 *******************************************************************/
void TextEditor::checkBraceMatch()
{
#ifdef __WXMAC__
	bool refresh = false;
#else
	bool refresh = true;
#endif

	// Ignore if cursor position hasn't changed since the last check
	if (GetCurrentPos() == bm_cursor_last_pos)
		return;

	bm_cursor_last_pos = GetCurrentPos();

	// Check for brace match at current position
	int bracematch = BraceMatch(GetCurrentPos());
	if (bracematch != wxSTC_INVALID_POSITION)
	{
		BraceHighlight(GetCurrentPos(), bracematch);
		if (refresh)
		{
			Refresh();
			Update();
		}
		return;
	}

	// No match, check for match at previous position
	bracematch = BraceMatch(GetCurrentPos() - 1);
	if (bracematch != wxSTC_INVALID_POSITION)
	{
		BraceHighlight(GetCurrentPos() - 1, bracematch);
		if (refresh)
		{
			Refresh();
			Update();
		}
		return;
	}

	// No match at all, clear any previous brace match
	BraceHighlight(-1, -1);
	if (refresh)
	{
		Refresh();
		Update();
	}
}

/* TextEditor::openCalltip
 * Opens a calltip for the function name before [pos]. Returns false
 * if the word before [pos] was not a function name, true otherwise
 *******************************************************************/
bool TextEditor::openCalltip(int pos, int arg)
{
	// Don't bother if no language
	if (!language)
		return false;

	// Get start of word before bracket
	int start = WordStartPosition(pos - 1, false);

	// Get word before bracket
	string word = GetTextRange(WordStartPosition(start, true), WordEndPosition(start, true));

	// Get matching language function (if any)
	TLFunction* func = language->getFunction(word);

	// Show calltip if it's a function
	if (func && func->nArgSets() > 0)
	{
		CallTipShow(start, func->generateCallTipString());
		ct_function = func;
		ct_argset = 0;
		ct_start = pos;

		// Highlight arg
		point2_t arg_ext = ct_function->getArgTextExtent(arg);
		CallTipSetHighlight(arg_ext.x, arg_ext.y);

		return true;
	}
	else
	{
		ct_function = NULL;
		return false;
	}
}

/* TextEditor::updateCalltip
 * Updates the current calltip, or attempts to open one if none is
 * currently showing
 *******************************************************************/
void TextEditor::updateCalltip()
{
	// Don't bother if no language
	if (!language)
		return;

	if (!CallTipActive())
	{
		// No calltip currently showing, check if we're in a function
		int pos = GetCurrentPos() - 1;
		while (pos >= 0)
		{
			// Get character
			int chr = GetCharAt(pos);

			// If we find a closing bracket, skip to matching brace
			if (chr == ')')
			{
				while (pos >= 0 && chr != '(')
				{
					pos--;
					chr = GetCharAt(pos);
				}
				pos--;
				continue;
			}

			// If we find an opening bracket, try to open a calltip
			if (chr == '(')
			{
				if (!openCalltip(pos))
					return;
				else
					break;
			}

			// Go to previous character
			pos--;
		}
	}

	if (ct_function)
	{
		// Calltip currently showing, determine what arg we're at
		int pos = ct_start+1;
		int arg = 0;
		while (pos < GetCurrentPos() && pos < GetTextLength())
		{
			// Get character
			int chr = GetCharAt(pos);

			// If it's an opening brace, skip until closing (ie skip a function as an arg)
			if (chr == '(')
			{
				while (chr != ')')
				{
					// Exit if we get to the current position or the end of the text
					if (pos == GetCurrentPos() || pos == GetTextLength()-1)
						break;

					// Get next character
					pos++;
					chr = GetCharAt(pos);
				}

				pos++;
				continue;
			}

			// If it's a comma, increment arg
			if (chr == ',')
				arg++;

			// If it's a closing brace, we're outside the function, so cancel the calltip
			if (chr == ')')
			{
				CallTipCancel();
				ct_function = NULL;
				return;
			}

			// Go to next character
			pos++;
		}

		// Update calltip string with the selected arg set and the current arg highlighted
		CallTipShow(ct_start, ct_function->generateCallTipString(ct_argset));
		point2_t arg_ext = ct_function->getArgTextExtent(arg, ct_argset);
		CallTipSetHighlight(arg_ext.x, arg_ext.y);
	}
}

/* TextEditor::openJumpToDialog
 * Initialises and opens the 'Jump To' dialog
 *******************************************************************/
void TextEditor::openJumpToDialog()
{
	// Can't do this without a language definition or defined blocks
	if (!language || language->nJumpBlocks() == 0)
		return;

	// --- Scan for functions/scripts ---
	Tokenizer tz;
	vector<jp_t> jump_points;
	tz.openString(GetText());

	string token = tz.getToken();
	while (!tz.atEnd())
	{
		if (token == "{")
		{
			// Skip block
			while (!tz.atEnd() && token != "}")
				token = tz.getToken();
		}

		for (unsigned a = 0; a < language->nJumpBlocks(); a++)
		{
			// Get jump block keyword
			string block = language->jumpBlock(a);
			long skip = 0;
			if (block.Contains(":"))
			{
				wxArrayString sp = wxSplit(block, ':');
				sp.back().ToLong(&skip);
				block = sp[0];
			}

			if (S_CMPNOCASE(token, block))
			{
				string name = tz.getToken();
				for (int s = 0; s < skip; s++)
					name = tz.getToken();

				for (unsigned i = 0; i < language->nJBIgnore(); ++i)
					if (S_CMPNOCASE(name, language->jBIgnore(i)))
						name = tz.getToken();

				// Numbered block, add block name
				if (name.IsNumber())
					name = S_FMT("%s %s", language->jumpBlock(a), name);
				// Unnamed block, use block name
				if (name == "{" || name == ";")
					name = language->jumpBlock(a);

				// Create jump point
				jp_t jp;
				jp.name = name;
				jp.line = tz.lineNo() - 1;
				jump_points.push_back(jp);
			}
		}

		token = tz.getToken();
	}

	// Do nothing if no jump points
	if (jump_points.size() == 0)
		return;


	// --- Setup/show dialog ---
	wxDialog dlg(this, -1, "Jump To...");
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	// Add Jump to dropdown
	wxChoice* choice_jump_to = new wxChoice(&dlg, -1);
	sizer->Add(choice_jump_to, 0, wxEXPAND|wxALL, 4);
	for (unsigned a = 0; a < jump_points.size(); a++)
		choice_jump_to->Append(jump_points[a].name);
	choice_jump_to->SetSelection(0);

	// Add dialog buttons
	sizer->Add(dlg.CreateButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxLEFT|wxRIGHT|wxBOTTOM, 4);

	// Show dialog
	dlg.SetInitialSize(wxSize(250, -1));
	dlg.CenterOnParent();
	if (dlg.ShowModal() == wxID_OK)
	{
		int selection = choice_jump_to->GetSelection();
		if (selection >= 0 && selection < (int)jump_points.size())
		{
			// Get line number
			int line = jump_points[selection].line;

			// Move to line
			int pos = GetLineEndPosition(line);
			SetCurrentPos(pos);
			SetSelection(pos, pos);
			SetFirstVisibleLine(line);
			SetFocus();
		}
	}
}

/* TextEditor::foldAll
 * Folds or unfolds all code folding levels, depending on [fold]
 *******************************************************************/
void TextEditor::foldAll(bool fold)
{
#if (wxMAJOR_VERSION >= 3 && wxMINOR_VERSION >= 1)
	// FoldAll is only available in wxWidgets 3.1+
	FoldAll(fold ? wxSTC_FOLDACTION_CONTRACT : wxSTC_FOLDACTION_EXPAND);
#else
	for (int a = 0; a < GetNumberOfLines(); a++)
	{
		int level = GetFoldLevel(a);
		if ((level & wxSTC_FOLDLEVELHEADERFLAG) > 0 &&
			GetFoldExpanded(a) == fold)
			ToggleFold(a);
	}
#endif
}

/* TextEditor::setupFolding
 * Sets up code folding options
 *******************************************************************/
void TextEditor::setupFolding()
{
	if (txed_fold_enable)
	{
		SetProperty("fold", "1");

		if (txed_fold_preprocessor)
			SetProperty("fold.preprocessor", "1");

		if (txed_fold_comments)
			SetProperty("fold.comment", "1");
	}
}


/*******************************************************************
 * TEXTEDITOR CLASS EVENTS
 *******************************************************************/

/* TextEditor::onKeyDown
 * Called when a key is pressed
 *******************************************************************/
void TextEditor::onKeyDown(wxKeyEvent& e)
{
	// Check if keypress matches any keybinds
	wxArrayString binds = KeyBind::getBinds(KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers()));

	// Go through matching binds
	bool handled = false;
	for (unsigned a = 0; a < binds.size(); a++)
	{
		string name = binds[a];

		// Open/update calltip
		if (name == "ted_calltip")
		{
			updateCalltip();
			handled = true;
		}

		// Autocomplete
		else if (name == "ted_autocomplete")
		{
			// Get word before cursor
			string word = GetTextRange(WordStartPosition(GetCurrentPos(), true), GetCurrentPos());

			// If a language is loaded, bring up autocompletion list
			if (language)
			{
				autocomp_list = language->getAutocompletionList(word);
				AutoCompShow(word.size(), autocomp_list);
			}

			handled = true;
		}

		// Find/replace
		else if (name == "ted_findreplace")
		{
			showFindReplacePanel();
			handled = true;
		}

		// Find next
		else if (name == "ted_findnext")
		{
			if (panel_fr && panel_fr->IsShown())
				findNext(panel_fr->getFindText(), panel_fr->getFindFlags());

			handled = true;
		}

		// Find previous
		else if (name == "ted_findprev")
		{
			if (panel_fr && panel_fr->IsShown())
				findPrev(panel_fr->getFindText(), panel_fr->getFindFlags());

			handled = true;
		}

		// Replace next
		else if (name == "ted_replacenext")
		{
			if (panel_fr && panel_fr->IsShown())
				replaceCurrent(panel_fr->getFindText(), panel_fr->getReplaceText(), panel_fr->getFindFlags());

			handled = true;
		}

		// Replace all
		else if (name == "ted_replaceall")
		{
			if (panel_fr && panel_fr->IsShown())
				replaceAll(panel_fr->getFindText(), panel_fr->getReplaceText(), panel_fr->getFindFlags());

			handled = true;
		}

		// Jump to
		else if (name == "ted_jumpto")
		{
			openJumpToDialog();
			handled = true;
		}

		// Fold all
		else if (name == "ted_fold_foldall")
		{
			foldAll(true);
			handled = true;
		}

		// Unfold all
		else if (name == "ted_fold_unfoldall")
		{
			foldAll(false);
			handled = true;
		}
	}

	// Check for esc key
	if (!handled && e.GetKeyCode() == WXK_ESCAPE)
	{
		// Hide F+R panel if showing
		if (panel_fr && panel_fr->IsShown())
			showFindReplacePanel(false);
	}

#ifdef __WXMSW__
	Colourise(GetCurrentPos(), GetLineEndPosition(GetCurrentLine()));
#endif
	
#ifdef __APPLE__
	if (!handled) {
		const int  keyCode =   e.GetKeyCode();
		const bool shiftDown = e.ShiftDown();

		if (e.ControlDown()) {
			if (WXK_LEFT == keyCode) {
				if (shiftDown) {
					HomeExtend();
				}
				else {
					Home();
				}

				handled = true;
			}
			else if (WXK_RIGHT == keyCode) {
				if (shiftDown) {
					LineEndExtend();
				}
				else {
					LineEnd();
				}

				handled = true;
			}
			else if (WXK_UP == keyCode) {
				if (shiftDown) {
					DocumentStartExtend();
				}
				else {
					DocumentStart();
				}

				handled = true;
			}
			else if (WXK_DOWN == keyCode) {
				if (shiftDown) {
					DocumentEndExtend();
				}
				else {
					DocumentEnd();
				}

				handled = true;
			}
		}
		else if (e.RawControlDown()) {
			if (WXK_LEFT == keyCode) {
				if (shiftDown) {
					WordLeftExtend();
				}
				else {
					WordLeft();
				}

				handled = true;
			}
			else if (WXK_RIGHT == keyCode) {
				if (shiftDown) {
					WordRightExtend();
				}
				else {
					WordRight();
				}

				handled = true;
			}
		}
	}
#endif // __APPLE__

	if (!handled)
		e.Skip();
}

/* TextEditor::onKeyUp
 * Called when a key is released
 *******************************************************************/
void TextEditor::onKeyUp(wxKeyEvent& e)
{
	e.Skip();
}

/* TextEditor::onCharAdded
 * Called when a character is added to the text
 *******************************************************************/
void TextEditor::onCharAdded(wxStyledTextEvent& e)
{
	// Update line numbers margin width
	string numlines = S_FMT("0%d", GetNumberOfLines());
	SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, numlines));

	// Auto indent
	int currentLine = GetCurrentLine();
	if (txed_auto_indent && e.GetKey() == '\n')
	{
		// Get indentation amount
		int lineInd = 0;
		if (currentLine > 0)
			lineInd = GetLineIndentation(currentLine - 1);

		// Do auto-indent if needed
		if (lineInd != 0)
		{
			SetLineIndentation(currentLine, lineInd);

			// Skip to end of tabs
			while (1)
			{
				int chr = GetCharAt(GetCurrentPos());
				if (chr == '\t' || chr == ' ')
					GotoPos(GetCurrentPos()+1);
				else
					break;
			}
		}
	}

	// The following require a language to work
	if (language)
	{
		// Call tip
		if (e.GetKey() == '(' && txed_calltips_parenthesis)
		{
			openCalltip(GetCurrentPos());
		}

		// End call tip
		if (e.GetKey() == ')')
		{
			CallTipCancel();
		}

		// Comma, possibly update calltip
		if (e.GetKey() == ',' && txed_calltips_parenthesis)
		{
			//openCalltip(GetCurrentPos());
			//if (CallTipActive())
			updateCalltip();
		}
	}

	// Continue
	e.Skip();
}

/* TextEditor::onUpdateUI
 * Called when anything is modified in the text editor (cursor
 * position, styling, text, etc)
 *******************************************************************/
void TextEditor::onUpdateUI(wxStyledTextEvent& e)
{
	// Check for brace match
	if (txed_brace_match)
		checkBraceMatch();

	// If a calltip is open, update it
	if (CallTipActive())
		updateCalltip();

	e.Skip();
}

/* TextEditor::onCalltipClicked
 * Called when the current calltip is clicked on
 *******************************************************************/
void TextEditor::onCalltipClicked(wxStyledTextEvent& e)
{
	// Can't do anything without function
	if (!ct_function)
		return;

	// Argset up
	if (e.GetPosition() == 1)
	{
		if (ct_argset > 0)
		{
			ct_argset--;
			updateCalltip();
		}
	}

	// Argset down
	if (e.GetPosition() == 2)
	{
		if ((unsigned)ct_argset < ct_function->nArgSets() - 1)
		{
			ct_argset++;
			updateCalltip();
		}
	}
}

/* TextEditor::onMouseDwellStart
 * Called when the mouse pointer has 'dwelt' in one position for a
 * certain amount of time
 *******************************************************************/
void TextEditor::onMouseDwellStart(wxStyledTextEvent& e)
{
	if (wxTheApp->IsActive() && HasFocus() && !CallTipActive() && txed_calltips_mouse && e.GetPosition() >= 0)
		openCalltip(e.GetPosition(), -1);
}

/* TextEditor::onMouseDwellEnd
 * Called when a mouse 'dwell' is interrupted/ended
 *******************************************************************/
void TextEditor::onMouseDwellEnd(wxStyledTextEvent& e)
{
	if (!(ct_function && ct_function->nArgSets() > 1) || !wxTheApp->IsActive() || !HasFocus())
		CallTipCancel();
}

/* TextEditor::onMouseDown
 * Called when a mouse button is clicked
 *******************************************************************/
void TextEditor::onMouseDown(wxMouseEvent& e)
{
	e.Skip();

	// No language, no checks
	if (!language)
		return;

	// Check for ctrl+left (web lookup)
	if (e.LeftDown() && e.GetModifiers() == wxMOD_CMD)
	{
		int pos = CharPositionFromPointClose(e.GetX(), e.GetY());
		string word = GetTextRange(WordStartPosition(pos, true), WordEndPosition(pos, true));

		if (!word.IsEmpty())
		{
			// Check for keyword
			if (language->isKeyword(word))
			{
				string url = language->getKeywordLink();
				if (!url.IsEmpty())
				{
					url.Replace("%s", word);
					wxLaunchDefaultBrowser(url);
				}
			}

			// Check for constant
			else if (language->isConstant(word))
			{
				string url = language->getConstantLink();
				if (!url.IsEmpty())
				{
					url.Replace("%s", word);
					wxLaunchDefaultBrowser(url);
				}
			}

			// Check for function
			else if (language->isFunction(word))
			{
				string url = language->getFunctionLink();
				if (!url.IsEmpty())
				{
					url.Replace("%s", word);
					wxLaunchDefaultBrowser(url);
				}
			}

			CallTipCancel();
		}
	}

	if (e.RightDown() || e.LeftDown())
		CallTipCancel();
}

/* TextEditor::onFocusLoss
 * Called when the text editor loses focus
 *******************************************************************/
void TextEditor::onFocusLoss(wxFocusEvent& e)
{
	CallTipCancel();
	AutoCompCancel();
	e.Skip();
}

/* TextEditor::onActivate
 * Called when the text editor is activated/deactivated
 *******************************************************************/
void TextEditor::onActivate(wxActivateEvent& e)
{
	if (!e.GetActive())
		CallTipCancel();
}

/* TextEditor::onMarginClick
 * Called when a margin is clicked
 *******************************************************************/
void TextEditor::onMarginClick(wxStyledTextEvent& e)
{
	if (e.GetMargin() == 1)
	{
		int line = LineFromPosition(e.GetPosition());
		int level = GetFoldLevel(line);
		if ((level & wxSTC_FOLDLEVELHEADERFLAG) > 0)
			ToggleFold(line);
	}
}
