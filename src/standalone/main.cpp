/* Copyright 2013-2016 Matt Tytel
 *
 * helm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * helm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with helm.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "JuceHeader.h"
#include "helm_standalone_editor.h"

class HelmApplication : public JUCEApplication {
  public:
    class MainWindow : public DocumentWindow,
                       public ApplicationCommandTarget,
                       private AsyncUpdater {
    public:
      enum PatchCommands {
        kSave = 0x5001,
        kSaveAs,
        kOpen,
      };

      MainWindow(String name, bool visible = true) :
          DocumentWindow(name, Colours::lightgrey, DocumentWindow::closeButton) {
        setUsingNativeTitleBar(true);
        editor_ = new HelmStandaloneEditor();
        setContentOwned(editor_, true);
        setResizable(true, true);

        centreWithSize(getWidth(), getHeight());
        setVisible(visible);
        triggerAsyncUpdate();
      }

      void closeButtonPressed() override {
        JUCEApplication::getInstance()->systemRequestedQuit();
      }

      bool loadFile(File file) {
        bool success = editor_->loadFromFile(file);
        if (success)
          editor_->externalPatchLoaded(file);
        return success;
      }

      ApplicationCommandTarget* getNextCommandTarget() override {
        return findFirstTargetParentComponent();
      }

      void getAllCommands(Array<CommandID>& commands) override {
        commands.add(kSave);
        commands.add(kSaveAs);
        commands.add(kOpen);
      }

      void getCommandInfo(const CommandID commandID, ApplicationCommandInfo& result) override {
        if (commandID == kSave) {
          result.setInfo(TRANS("Save"), TRANS("Saves the current patch"), "Application", 0);
          result.defaultKeypresses.add(KeyPress('s', ModifierKeys::commandModifier, 0));
        }
        else if (commandID == kSaveAs) {
          result.setInfo(TRANS("Save As"), TRANS("Saves patch to a new file"), "Application", 0);
          ModifierKeys modifier = ModifierKeys::commandModifier | ModifierKeys::shiftModifier;
          result.defaultKeypresses.add(KeyPress('s', modifier, 0));
        }
        else if (commandID == kOpen) {
          result.setInfo(TRANS("Open"), TRANS("Opens a patch"), "Application", 0);
          result.defaultKeypresses.add(KeyPress('o', ModifierKeys::commandModifier, 0));
        }
      }

      void open() {
        File active_file = editor_->getActiveFile();
        FileChooser open_box("Open Patch", File(),
                             String("*.") + mopo::PATCH_EXTENSION);
        if (open_box.browseForFileToOpen())
          loadFile(open_box.getResult());
      }

      bool perform(const InvocationInfo& info) override {
        if (info.commandID == kSave) {
          if (!editor_->saveToActiveFile())
            editor_->exportToFile();
          grabKeyboardFocus();
          editor_->setFocus();
          return true;
        }
        if (info.commandID == kSaveAs) {
          editor_->exportToFile();
          grabKeyboardFocus();
          editor_->setFocus();
          return true;
        }
        if (info.commandID == kOpen) {
          open();
          grabKeyboardFocus();
          editor_->setFocus();
          return true;
        }

        return false;
      }

      void handleAsyncUpdate() override {
        command_manager_ = new ApplicationCommandManager();
        command_manager_->registerAllCommandsForTarget(JUCEApplication::getInstance());
        command_manager_->registerAllCommandsForTarget(this);
        addKeyListener(command_manager_->getKeyMappings());
        editor_->setFocus();
      }

    private:
      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
      HelmStandaloneEditor* editor_;
      ScopedPointer<ApplicationCommandManager> command_manager_;
    };

    HelmApplication() { }

    const String getApplicationName() override { return ProjectInfo::projectName; }
    const String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const String& command_line) override {
      if (command_line.contains(" --version ") || command_line.contains(" -v ")) {
        std::cout << getApplicationName() << " " << getApplicationVersion() << newLine;
        quit();
      }
      else if (command_line.contains(" --help ") || command_line.contains(" -h ")) {
        std::cout << "Usage:" << newLine;
        std::cout << "  " << getApplicationName().toLowerCase() << " [OPTION...]" << newLine << newLine;
        std::cout << getApplicationName() << " polyphonic, semi-modular synthesizer." << newLine << newLine;
        std::cout << "Help Options:" << newLine;
        std::cout << "  -h, --help                          Show help options" << newLine << newLine;
        std::cout << "Application Options:" << newLine;
        std::cout << "  -v, --version                       Show version information and exit" << newLine << newLine;
        quit();
      }
      else {
        bool visible = !command_line.contains("--headless");
        main_window_ = new MainWindow(getApplicationName(), visible);

        StringArray args = getCommandLineParameterArray();
        File file;

        for (int i = 0; i < args.size(); ++i) {
          if (args[i] != "" && args[i][0] != '-' && loadFromCommandLine(args[i]))
            return;
        }
      }
    }

    bool loadFromCommandLine(const String& command_line) {
      String file_path = command_line;
      if (file_path[0] == '"' && file_path[file_path.length() - 1] == '"')
        file_path = command_line.substring(1, command_line.length() - 1);
      File file = File::getCurrentWorkingDirectory().getChildFile(file_path);
      if (file.exists())
        return main_window_->loadFile(file);
      return false;
    }

    void shutdown() override {
      main_window_ = nullptr;
    }

    void systemRequestedQuit() override {
      quit();
    }

    void anotherInstanceStarted(const String& command_line) override {
      loadFromCommandLine(command_line);
    }
    
  private:
    ScopedPointer<MainWindow> main_window_;
};

START_JUCE_APPLICATION(HelmApplication)
