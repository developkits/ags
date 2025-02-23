using AGS.Types;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Text;
using System.Windows.Forms;
using AGS.Editor.TextProcessing;

namespace AGS.Editor
{
    public partial class DialogEditor : EditorContentPanel
    {
        private const string dialogKeyWords = "return stop";

        private const string FIND_COMMAND = "ScriptFind";
        private const string FIND_NEXT_COMMAND = "ScriptFindNext";
        private const string REPLACE_COMMAND = "ScriptReplace";
        private const string FIND_ALL_COMMAND = "ScriptFindAll";
        private const string REPLACE_ALL_COMMAND = "ScriptReplaceAll";

        private Dialog _dialog;
        private List<DialogOptionEditor> _optionPanes = new List<DialogOptionEditor>();
        private MenuCommands _extraMenu = new MenuCommands("&Edit", GUIController.FILE_MENU_ID);
        
        private string _lastSearchText = string.Empty;
        private bool _lastCaseSensitive = false;
        private AGSEditor _agsEditor;

        public DialogEditor(Dialog dialogToEdit, AGSEditor agsEditor)
        {
            InitializeComponent();
            _dialog = dialogToEdit;
            _agsEditor = agsEditor;

            _extraMenu.Commands.Add(new MenuCommand(FIND_COMMAND, "Find...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.F, "FindMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(FIND_NEXT_COMMAND, "Find next", System.Windows.Forms.Keys.F3, "FindNextMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(REPLACE_COMMAND, "Replace...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.E));
            _extraMenu.Commands.Add(MenuCommand.Separator);
            _extraMenu.Commands.Add(new MenuCommand(FIND_ALL_COMMAND, "Find All...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift | System.Windows.Forms.Keys.F, "FindMenuIcon"));
            _extraMenu.Commands.Add(new MenuCommand(REPLACE_ALL_COMMAND, "Replace All...", System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift | System.Windows.Forms.Keys.E));

            scintillaEditor.SetAsDialog();
            scintillaEditor.AutoCompleteEnabled = true;
            scintillaEditor.IgnoreLinesWithoutIndent = true;
			scintillaEditor.AutoSpaceAfterComma = false;
            scintillaEditor.CallTipsEnabled = true;
            scintillaEditor.FixedTypeForThisKeyword = "Dialog";
            scintillaEditor.SetFillupKeys(Constants.AUTOCOMPLETE_ACCEPT_KEYS);
            //scintillaEditor.SetKeyWords(dialogKeyWords);
            scintillaEditor.SetKeyWords(Constants.SCRIPT_KEY_WORDS);
            scintillaEditor.SetClassNamesList(BuildCharacterKeywords());
            scintillaEditor.SetAutoCompleteKeyWords(Constants.SCRIPT_KEY_WORDS);
            scintillaEditor.SetAutoCompleteSource(_dialog);
            scintillaEditor.SetText(dialogToEdit.Script);


            flowLayoutPanel1.Controls.Remove(btnNewOption);
            foreach (DialogOption option in dialogToEdit.Options)
            {
                DialogOptionEditor optionEditor = new DialogOptionEditor(option);
                _optionPanes.Add(optionEditor);
                flowLayoutPanel1.Controls.Add(optionEditor);
            }
            flowLayoutPanel1.Controls.Add(btnNewOption);
            flowLayoutPanel1.Controls.Add(btnDeleteOption);

            if (_dialog.Options.Count >= Dialog.MAX_OPTIONS_PER_DIALOG)
            {
                btnNewOption.Visible = false;
            }
            if (_dialog.Options.Count < 1)
            {
                btnDeleteOption.Visible = false;
            }
        }

        public ScintillaWrapper ScriptEditor
        {
            get { return scintillaEditor; }
        }

        public Dialog ItemToEdit
        {
            get { return _dialog; }
        }

        public MenuCommands ExtraMenu
        {
            get { return _extraMenu; }
        }

        protected override void OnKeyPressed(Keys keyData)
        {
            if (keyData.Equals(Keys.Escape))
            {
                FindReplace.CloseDialogIfNeeded();
            }
        }

        protected override string OnGetHelpKeyword()
        {
            return "Dialogs";
        }
        
        protected override void OnCommandClick(string command)
        {
            base.OnCommandClick(command);
            if ((command == FIND_COMMAND) || (command == REPLACE_COMMAND)
                || (command == FIND_ALL_COMMAND) || (command == REPLACE_ALL_COMMAND))
            {
                if (scintillaEditor.IsSomeSelectedText())
                {
                    _lastSearchText = scintillaEditor.SelectedText;
                }
                else _lastSearchText = string.Empty;
                ShowFindReplaceDialog(command == REPLACE_COMMAND || command == REPLACE_ALL_COMMAND,
                    command == FIND_ALL_COMMAND || command == REPLACE_ALL_COMMAND);
            }
            else if (command == FIND_NEXT_COMMAND)
            {
                if (_lastSearchText.Length > 0)
                {
                    scintillaEditor.FindNextOccurrence(_lastSearchText, _lastCaseSensitive, true);
                }
            }
        }

        private String BuildCharacterKeywords()
        {
            StringBuilder sb = new StringBuilder(300);
            foreach(Character c in _agsEditor.CurrentGame.Characters)
            {
                sb.Append(c.ScriptName.TrimStart('c') + " ");
                sb.Append(c.ScriptName.TrimStart('c').ToLower() + " ");
            }
            return sb.ToString();
        }

        public void SaveData()
        {
            _dialog.Script = scintillaEditor.GetText();
        }

		public void GoToScriptLine(ZoomToFileEventArgs evArgs)
		{
            if (evArgs.ZoomType == ZoomToFileZoomType.ZoomToCharacterPosition)
            {
                scintillaEditor.GoToPosition(evArgs.ZoomPosition);
            }
            else if (evArgs.ZoomType == ZoomToFileZoomType.ZoomToLineNumber)
            {
			    scintillaEditor.GoToLine(evArgs.ZoomPosition);
            }
			scintillaEditor.Focus();

            if (evArgs.IsDebugExecutionPoint)
            {
                scintillaEditor.ShowCurrentExecutionPoint(evArgs.ZoomPosition);
                if (evArgs.ErrorMessage != null)
                {
                    scintillaEditor.ShowErrorMessagePopup(evArgs.ErrorMessage);
                }
            }
		}

        public void RemoveExecutionPointMarker()
        {
            scintillaEditor.HideCurrentExecutionPoint();
            scintillaEditor.HideErrorMessagePopup();
        }

        private void ShowFindReplaceDialog(bool showReplace, bool showAll)
        {
            FindReplace findReplace = new FindReplace(_dialog, _agsEditor,
                _lastSearchText, _lastCaseSensitive);
            findReplace.ShowFindReplaceDialog(showReplace, showAll);
        }

        private void btnDeleteOption_Click(object sender, EventArgs e)
        {
            if (Factory.GUIController.ShowQuestion("Are you sure you want to delete the last option?") == DialogResult.Yes)
            {
                _dialog.Options.RemoveAt(_dialog.Options.Count - 1);
                flowLayoutPanel1.Controls.Remove(_optionPanes[_optionPanes.Count - 1]);
                _optionPanes.RemoveAt(_optionPanes.Count - 1);

                SaveData();

                if (_dialog.Options.Count < 1)
                {
                    btnDeleteOption.Visible = false;
                }
                else
                {
                    btnNewOption.Visible = true;
                }
            }
        }

        private void btnNewOption_Click(object sender, EventArgs e)
        {
            DialogOption newOption = new DialogOption();
            newOption.ID = _dialog.Options.Count + 1;
			if (_dialog.Options.Count > 0)
			{
				// Copy Show & Say settings from previous option
				newOption.Say = _dialog.Options[_dialog.Options.Count - 1].Say;
				newOption.Show = _dialog.Options[_dialog.Options.Count - 1].Show;
			}
			else
			{
				newOption.Say = true;
				newOption.Show = true;
			}
            _dialog.Options.Add(newOption);
            DialogOptionEditor newEditor = new DialogOptionEditor(newOption);
            _optionPanes.Add(newEditor);
            flowLayoutPanel1.Controls.Remove(btnNewOption);
            flowLayoutPanel1.Controls.Remove(btnDeleteOption);
            flowLayoutPanel1.Controls.Add(newEditor);
            flowLayoutPanel1.Controls.Add(btnNewOption);
            flowLayoutPanel1.Controls.Add(btnDeleteOption);
            newEditor.Focus();

            if (_dialog.Options.Count >= Dialog.MAX_OPTIONS_PER_DIALOG)
            {
                btnNewOption.Visible = false;
            }
            else
            {
                btnDeleteOption.Visible = true;
            }

            SaveData();

            // Ensure there is an entry point in the script for this
            if (!_dialog.Script.Contains(Environment.NewLine + "@" + newOption.ID))
            {
                if (!_dialog.Script.EndsWith(Environment.NewLine))
                {
                    _dialog.Script += Environment.NewLine;
                }
                _dialog.Script += "@" + newOption.ID + Environment.NewLine + "return" + Environment.NewLine;
                scintillaEditor.SetText(_dialog.Script);
            }
        }


    }
}
