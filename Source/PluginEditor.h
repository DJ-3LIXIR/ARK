/*
  ==============================================================================
    This file contains the basic framework code for a JUCE plugin editor.
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Custom Look and Feel for Keyboard
class KeyboardLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KeyboardLookAndFeel()
    {
        setColour(juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colours::white);
        setColour(juce::MidiKeyboardComponent::blackNoteColourId, juce::Colours::black);
        setColour(juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour(0xff2a2a2a));
        setColour(juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, juce::Colour(0xffD4A017).withAlpha(0.3f));
        setColour(juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour(0xffD4A017).withAlpha(0.6f));
    }
};

//==============================================================================
// Custom Look and Feel for Knobs
class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override
    {
        if (width < 16 || height < 16)
            return;

        auto radius = juce::jmax(4.0f, juce::jmin(width / 2.0f, height / 2.0f) - 4.0f);
        auto centerX = x + width * 0.5f;
        auto centerY = y + height * 0.5f;
        auto rx = centerX - radius;
        auto ry = centerY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillEllipse(rx, ry, rw, rw);

        float inset  = juce::jmin(3.0f, radius * 0.25f);
        float innerW = juce::jmax(2.0f, rw - (inset * 2.0f));

        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillEllipse(rx + inset, ry + inset, innerW, innerW);

        if (radius > 4.0f)
        {
            juce::Path valueArc;
            float arcRadius = juce::jmax(2.0f, radius - 2.0f);
            valueArc.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                                  0.0f, rotaryStartAngle, angle, true);
            g.setColour(juce::Colour(0xffD4A017));
            g.strokePath(valueArc, juce::PathStrokeType(juce::jmin(3.0f, radius * 0.3f)));
        }

        juce::Path pointer;
        auto pointerLength    = radius * 0.6f;
        auto pointerThickness = juce::jmin(3.0f, radius * 0.25f);
        pointer.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centerX, centerY));
        g.setColour(juce::Colour(0xffD4A017));
        g.fillPath(pointer);

        float dotSize = juce::jmin(8.0f, radius * 0.5f);
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillEllipse(centerX - dotSize * 0.5f, centerY - dotSize * 0.5f, dotSize, dotSize);
    }
};

//==============================================================================
// Custom Look and Feel for Waveform Buttons
class WaveBtnLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Font getTextButtonFont(juce::TextButton&, int /*buttonHeight*/) override
    {
        return juce::Font(juce::FontOptions(11.0f).withStyle("Bold"));
    }
};

//==============================================================================
// LookAndFeel exclusively for the small MASTER header dials
class MasterDialLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider&) override
    {
        const float cx     = x + width  * 0.5f;
        const float cy     = y + height * 0.5f;
        const float radius = juce::jmin(width, height) * 0.5f - 1.0f;
        const float angle  = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.0f);

        const float arcRadius = radius - 3.0f;
        juce::Path arc;
        arc.addCentredArc(cx, cy, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
        g.setColour(juce::Colour(0xffD4A017));
        g.strokePath(arc, juce::PathStrokeType(1.5f));

        const float pLen   = radius * 0.55f;
        const float pThick = 1.5f;
        juce::Path pointer;
        pointer.addRectangle(-pThick * 0.5f, -(radius - 2.0f), pThick, pLen);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(cx, cy));
        g.setColour(juce::Colour(0xffD4A017));
        g.fillPath(pointer);

        g.setColour(juce::Colour(0xff1e1e1e));
        g.fillEllipse(cx - 2.5f, cy - 2.5f, 5.0f, 5.0f);
    }
};

//==============================================================================
// Preset Browser Overlay
//
// Displays the semi-transparent preset browser panel over the entire editor.
// Wired to the processor so it can read real folder/preset lists from disk.
// The "User Presets" folder always appears first; any additional user-created
// folders appear below it.  Presets inside the selected folder are listed on
// the right panel and are clickable to load.
//==============================================================================
class PresetBrowserOverlay : public juce::Component
{
public:
    // Callbacks — set by the editor before the overlay is shown
    std::function<void(const juce::String& presetName,
                       const juce::String& folderName)> onPresetSelected;
    std::function<void()> onDismiss;

    // Call this once after construction so the overlay can query preset data
    void setProcessor (ARKAudioProcessor* p)
    {
        processor = p;
        refreshData();
    }

    // Re-reads folder and preset lists from the processor — call after save
    void refreshData()
    {
        if (processor == nullptr) return;

        // getPresetFolders() returns stock folders first, then User Presets, then extras
        folderNames = processor->getPresetFolders();

        // Clamp selectedFolderIndex into range after a refresh
        selectedFolderIndex = juce::jlimit (0, juce::jmax (0, folderNames.size() - 1),
                                            selectedFolderIndex);
        refreshPresetList();
        rebuildFolderButtons();
        resized();
        repaint();
    }

    PresetBrowserOverlay()
    {
        selectedFolderIndex = 0;

        // ── Close button ──────────────────────────────────────────────────────
        closeBtn.setButtonText ("X");
        closeBtn.setColour (juce::TextButton::buttonColourId,  juce::Colour(0xff2a2a2a));
        closeBtn.setColour (juce::TextButton::textColourOffId, juce::Colour(0xffD4A017));
        closeBtn.onClick = [this] { if (onDismiss) onDismiss(); };
        addAndMakeVisible (closeBtn);

        // ── Search bar ────────────────────────────────────────────────────────
        searchBox.setTextToShowWhenEmpty ("Search presets...", juce::Colours::grey);
        searchBox.setColour (juce::TextEditor::backgroundColourId, juce::Colour(0xff1a1a1a));
        searchBox.setColour (juce::TextEditor::textColourId,       juce::Colour(0xffD4A017));
        searchBox.setColour (juce::TextEditor::outlineColourId,    juce::Colour(0xffD4A017).withAlpha(0.4f));
        searchBox.onTextChange = [this] { refreshPresetList(); repaint(); };
        addAndMakeVisible (searchBox);

        // ── Sort combo ────────────────────────────────────────────────────────
        sortBox.addItem ("Sort: A-Z",         1);
        sortBox.addItem ("Sort: Z-A",         2);
        sortBox.addItem ("Sort: Newest",      3);
        sortBox.addSeparator();
        sortBox.addItem ("Filter: Pads",      4);
        sortBox.addItem ("Filter: Keys",      5);
        sortBox.addItem ("Filter: Leads",     6);
        sortBox.addItem ("Filter: Bass",      7);
        sortBox.addItem ("Filter: Horns",     8);
        sortBox.addItem ("Filter: Arp",       9);
        sortBox.addItem ("Filter: Strings",   10);
        sortBox.addItem ("Filter: SFX",       11);
        sortBox.setSelectedId (1, juce::dontSendNotification);
        sortBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour(0xff1a1a1a));
        sortBox.setColour (juce::ComboBox::textColourId,       juce::Colour(0xffD4A017));
        sortBox.setColour (juce::ComboBox::outlineColourId,    juce::Colour(0xffD4A017).withAlpha(0.4f));
        sortBox.onChange = [this] { refreshPresetList(); repaint(); };
        addAndMakeVisible (sortBox);

        // ── "New Folder" button ───────────────────────────────────────────────
        newFolderBtn.setButtonText ("+ New Folder");
        newFolderBtn.setColour (juce::TextButton::buttonColourId,  juce::Colour(0xff1e1e1e));
        newFolderBtn.setColour (juce::TextButton::textColourOffId, juce::Colour(0xffD4A017));
        newFolderBtn.onClick = [this] { promptCreateFolder(); };
        addAndMakeVisible (newFolderBtn);

        // ── Pagination buttons ───────────────────────────────────────────────
        prevPageBtn.setButtonText ("< Prev");
        prevPageBtn.setColour (juce::TextButton::buttonColourId,  juce::Colour(0xff1e1e1e));
        prevPageBtn.setColour (juce::TextButton::textColourOffId, juce::Colour(0xffD4A017));
        prevPageBtn.onClick = [this]
        {
            if (currentPage > 0)
            {
                --currentPage;
                hoveredPresetIndex = -1;
                prevPageBtn.setEnabled (currentPage > 0);
                nextPageBtn.setEnabled (currentPage < totalPages - 1);
                repaint();
            }
        };
        addAndMakeVisible (prevPageBtn);

        nextPageBtn.setButtonText ("Next >");
        nextPageBtn.setColour (juce::TextButton::buttonColourId,  juce::Colour(0xff1e1e1e));
        nextPageBtn.setColour (juce::TextButton::textColourOffId, juce::Colour(0xffD4A017));
        nextPageBtn.onClick = [this]
        {
            if (currentPage < totalPages - 1)
            {
                ++currentPage;
                hoveredPresetIndex = -1;
                prevPageBtn.setEnabled (currentPage > 0);
                nextPageBtn.setEnabled (currentPage < totalPages - 1);
                repaint();
            }
        };
        addAndMakeVisible (nextPageBtn);

        // Initial placeholder for folder buttons — real ones built in refreshData()
        folderNames = { "Ethereal", "Circuit", "Analog Soul",
                        "Dark Matter", "Modular", "Pure", "Yamaha DX7 Library", "User Presets" };
        rebuildFolderButtons();
    }

    void paint (juce::Graphics& g) override
    {
        // Semi-transparent backdrop
        g.setColour (juce::Colours::black.withAlpha(0.75f));
        g.fillAll();

        auto panel = getLocalBounds().reduced (80, 60);

        // Main panel background
        g.setColour (juce::Colour(0xff141414));
        g.fillRoundedRectangle (panel.toFloat(), 8.0f);
        g.setColour (juce::Colour(0xffD4A017));
        g.drawRoundedRectangle (panel.toFloat(), 8.0f, 1.5f);

        // Title
        g.setColour (juce::Colour(0xffD4A017));
        g.setFont (juce::Font(juce::FontOptions(18.0f).withStyle("Bold")));
        g.drawText ("PRESET BROWSER", panel.getX() + 16, panel.getY() + 12, 300, 24,
                    juce::Justification::centredLeft);

        // Left panel label
        g.setColour (juce::Colours::white.withAlpha(0.4f));
        g.setFont (juce::Font(juce::FontOptions(10.0f)));
        g.drawText ("FOLDERS", panel.getX() + 10, panel.getY() + 46, 200, 14,
                    juce::Justification::centredLeft);

        // Divider between left and right
        int divX = panel.getX() + 200;
        g.setColour (juce::Colour(0xffD4A017).withAlpha(0.3f));
        g.fillRect (divX, panel.getY() + 50, 1, panel.getHeight() - 60);

        // Right panel — preset list
        int rightX = divX + 12;
        int rightW = panel.getRight() - rightX - 12;
        int rightY = panel.getY() + 56;

        if (visiblePresets.isEmpty())
        {
            g.setColour (juce::Colours::white.withAlpha(0.2f));
            g.setFont (juce::Font(juce::FontOptions(13.0f).withStyle("Italic")));
            g.drawText ("No presets saved here yet.",
                        rightX, panel.getCentreY() - 12, rightW, 24,
                        juce::Justification::centred);
        }
        else
        {
            // Column header
            g.setColour (juce::Colours::white.withAlpha(0.35f));
            g.setFont (juce::Font(juce::FontOptions(10.0f)));
            g.drawText ("PRESETS", rightX, rightY, rightW, 14,
                        juce::Justification::centredLeft);

            // ── Grid of presets ──────────────────────────────────────────────
            const int headerH = 18;
            const int rowH    = 26;
            const int spacing = 2;
            int gridTop = rightY + headerH;

            int startIdx = currentPage * presetsPerPage;
            int endIdx   = juce::jmin (startIdx + presetsPerPage, visiblePresets.size());

            g.setFont (juce::Font(juce::FontOptions(13.0f)));

            for (int i = startIdx; i < endIdx; ++i)
            {
                int localIdx = i - startIdx;
                int col = localIdx % numColumns;
                int row = localIdx / numColumns;

                int cellX = rightX + col * cellWidth;
                int cellY = gridTop + row * (rowH + spacing);

                juce::Rectangle<int> cell (cellX, cellY, cellWidth, rowH);

                if (i == hoveredPresetIndex)
                {
                    g.setColour (juce::Colour(0xffD4A017).withAlpha(0.15f));
                    g.fillRoundedRectangle (cell.toFloat(), 3.0f);
                }

                g.setColour (juce::Colours::white.withAlpha(0.85f));
                g.drawText (visiblePresets[i], cell.reduced(6, 0),
                            juce::Justification::centredLeft, true);
            }

            // ── Pagination indicator ─────────────────────────────────────────
            if (totalPages > 1)
            {
                const int paginationH = 30;
                int pageBarY = panel.getBottom() - 12 - paginationH + 4;

                g.setColour (juce::Colour(0xffD4A017).withAlpha(0.8f));
                g.setFont (juce::Font(juce::FontOptions(12.0f)));
                g.drawText ("Page " + juce::String(currentPage + 1) + " of " + juce::String(totalPages),
                            rightX + 70, pageBarY, rightW - 140, 22,
                            juce::Justification::centred);
            }
        }
    }

    void resized() override
    {
        auto panel = getLocalBounds().reduced (80, 60);

        // Close button — top-right of panel
        closeBtn.setBounds (panel.getRight() - 36, panel.getY() + 8, 28, 28);

        // Search bar
        searchBox.setBounds (panel.getX() + 8, panel.getY() + 50, 182, 24);

        // Sort combo — below search
        sortBox.setBounds (panel.getX() + 8, panel.getY() + 80, 182, 22);

        // Folder buttons — stacked below sort
        int btnY = panel.getY() + 112;
        for (auto* btn : folderBtns)
        {
            btn->setBounds (panel.getX() + 8, btnY, 182, 26);
            btnY += 30;
        }

        // New Folder button — below folder list, left panel
        newFolderBtn.setBounds (panel.getX() + 8, btnY + 4, 182, 24);

        // ── Compute grid dimensions for right panel ─────────────────────────
        int divX   = panel.getX() + 200;
        int rightX = divX + 12;
        int rightW = panel.getRight() - rightX - 12;

        const int headerH     = 18;
        const int paginationH = 30;
        const int rowH        = 26;
        const int spacing     = 2;

        int gridTop    = panel.getY() + 56 + headerH;
        int gridBottom = panel.getBottom() - 12 - paginationH;
        int gridH      = gridBottom - gridTop;

        numColumns = juce::jmax (1, rightW / 150);
        cellWidth  = rightW / numColumns;

        numRows = juce::jmax (1, gridH / (rowH + spacing));

        presetsPerPage = numColumns * numRows;
        totalPages     = (visiblePresets.size() + presetsPerPage - 1) / juce::jmax (1, presetsPerPage);
        if (totalPages < 1) totalPages = 1;

        currentPage = juce::jlimit (0, juce::jmax (0, totalPages - 1), currentPage);

        // Position pagination buttons
        int pageBarY = gridBottom + 4;
        prevPageBtn.setBounds (rightX, pageBarY, 70, 22);
        nextPageBtn.setBounds (rightX + rightW - 70, pageBarY, 70, 22);

        bool needsPagination = totalPages > 1;
        prevPageBtn.setVisible (needsPagination);
        nextPageBtn.setVisible (needsPagination);
        prevPageBtn.setEnabled (currentPage > 0);
        nextPageBtn.setEnabled (currentPage < totalPages - 1);
    }

    // Hit-test for preset row clicks on the right panel
    void mouseMove (const juce::MouseEvent& e) override
    {
        int newHover = presetIndexAtPoint (e.getPosition());
        if (newHover != hoveredPresetIndex)
        {
            hoveredPresetIndex = newHover;
            repaint();
        }
    }

    void mouseExit (const juce::MouseEvent&) override
    {
        hoveredPresetIndex = -1;
        repaint();
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        int idx = presetIndexAtPoint (e.getPosition());
        if (idx >= 0 && idx < visiblePresets.size())
        {
            if (onPresetSelected)
                onPresetSelected (visiblePresets[idx], folderNames[selectedFolderIndex]);
        }
    }

private:
    ARKAudioProcessor* processor = nullptr;

    juce::TextButton          closeBtn;
    juce::TextEditor          searchBox;
    juce::ComboBox            sortBox;
    juce::TextButton          newFolderBtn;
    juce::OwnedArray<juce::TextButton> folderBtns;

    juce::StringArray folderNames;
    juce::StringArray allPresetsInFolder;  // unfiltered list for selected folder
    juce::StringArray visiblePresets;       // filtered + sorted list shown in right panel

    int selectedFolderIndex = 0;
    int hoveredPresetIndex  = -1;

    // ── Grid & Pagination ────────────────────────────────────────────────
    int currentPage    = 0;
    int numColumns     = 1;
    int numRows        = 1;
    int presetsPerPage = 1;
    int totalPages     = 1;
    int cellWidth      = 150;

    juce::TextButton prevPageBtn;
    juce::TextButton nextPageBtn;

    // Re-reads the preset list for the currently selected folder, applies
    // search filter and sort order, then triggers a repaint.
    void refreshPresetList()
    {
        if (processor != nullptr && selectedFolderIndex < folderNames.size())
            allPresetsInFolder = processor->getPresetsInFolder (folderNames[selectedFolderIndex]);
        else
            allPresetsInFolder.clear();

        juce::String query  = searchBox.getText().trim().toLowerCase();
        int          sortId = sortBox.getSelectedId();

        // Category suffix map: sortId → suffix to match at end of preset name
        juce::String categoryFilter;
        switch (sortId)
        {
            case 4:  categoryFilter = "_Pad";     break;
            case 5:  categoryFilter = "_Keys";    break;
            case 6:  categoryFilter = "_Lead";    break;
            case 7:  categoryFilter = "_Bass";    break;
            case 8:  categoryFilter = "_Horn";    break;
            case 9:  categoryFilter = "_Arp";     break;
            case 10: categoryFilter = "_Strings"; break;
            case 11: categoryFilter = "_SFX";     break;
            default: categoryFilter = "";         break;
        }

        visiblePresets.clear();

        for (auto& name : allPresetsInFolder)
        {
            // Search filter
            if (query.isNotEmpty() && !name.toLowerCase().contains (query))
                continue;

            // Category filter — match suffix (case-insensitive)
            if (categoryFilter.isNotEmpty() &&
                !name.toLowerCase().endsWith (categoryFilter.toLowerCase()))
                continue;

            visiblePresets.add (name);
        }

        // Sort (only A-Z / Z-A / Newest apply; category filters show A-Z by default)
        if (sortId == 1 || sortId >= 4)
            visiblePresets.sortNatural();
        else if (sortId == 2)
        {
            visiblePresets.sortNatural();
            juce::StringArray rev;
            for (int i = visiblePresets.size() - 1; i >= 0; --i)
                rev.add (visiblePresets[i]);
            visiblePresets = rev;
        }
        // sortId == 3 (Newest) keeps disk order

        hoveredPresetIndex = -1;

        // Reset to page 0 when the preset list changes
        currentPage = 0;

        // Recompute totalPages so paint/pagination stay in sync
        if (presetsPerPage > 0)
            totalPages = (visiblePresets.size() + presetsPerPage - 1) / presetsPerPage;
        else
            totalPages = 1;
        if (totalPages < 1) totalPages = 1;
    }

    // Rebuilds the left-panel folder button list from folderNames
    void rebuildFolderButtons()
    {
        folderBtns.clear();

        for (int i = 0; i < folderNames.size(); ++i)
        {
            auto* btn = folderBtns.add (new juce::TextButton (folderNames[i]));
            btn->setClickingTogglesState (false);
            btn->setColour (juce::TextButton::buttonColourId,  juce::Colour(0xff1e1e1e));
            btn->setColour (juce::TextButton::buttonOnColourId,juce::Colour(0xffD4A017));
            btn->setColour (juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.8f));
            btn->setColour (juce::TextButton::textColourOnId,  juce::Colours::black);

            const int idx = i;
            btn->onClick = [this, idx]
            {
                selectedFolderIndex = idx;
                refreshPresetList();
                repaint();
            };
            addAndMakeVisible (btn);
        }
    }

    // Opens an AlertWindow so the user can name a new folder
    void promptCreateFolder()
    {
        auto dialog = std::make_shared<juce::AlertWindow> (
            "New Folder", "Enter folder name:", juce::MessageBoxIconType::NoIcon);
        dialog->addTextEditor ("name", "", "");
        dialog->addButton ("Create", 1);
        dialog->addButton ("Cancel", 0);

        dialog->enterModalState (true,
            juce::ModalCallbackFunction::create ([this, dialog](int result)
            {
                if (result == 1 && processor != nullptr)
                {
                    juce::String name = dialog->getTextEditorContents ("name").trim();
                    if (name.isNotEmpty())
                    {
                        processor->createPresetFolder (name);
                        refreshData();
                    }
                }
            }), false);
    }

    // Returns the preset list index for a point in the component, or -1
    int presetIndexAtPoint (juce::Point<int> pt) const
    {
        auto panel  = getLocalBounds().reduced (80, 60);
        int  divX   = panel.getX() + 200;
        int  rightX = divX + 12;
        int  rightW = panel.getRight() - rightX - 12;

        const int headerH = 18;
        const int rowH    = 26;
        const int spacing = 2;
        int gridTop = panel.getY() + 56 + headerH;

        // Out of bounds horizontally?
        if (pt.x < rightX || pt.x > rightX + rightW) return -1;

        // Which column?
        int col = (pt.x - rightX) / cellWidth;
        if (col < 0 || col >= numColumns) return -1;

        // Which row?
        int relY = pt.y - gridTop;
        if (relY < 0) return -1;

        int row = relY / (rowH + spacing);
        // Check we're actually inside the cell, not in the spacing gap
        if (relY - row * (rowH + spacing) >= rowH) return -1;
        if (row < 0 || row >= numRows) return -1;

        // Convert to absolute index in visiblePresets
        int localIdx = row * numColumns + col;
        int absIdx   = currentPage * presetsPerPage + localIdx;

        if (absIdx < 0 || absIdx >= visiblePresets.size()) return -1;
        return absIdx;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetBrowserOverlay)
};

//==============================================================================
class ARKAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ARKAudioProcessorEditor (ARKAudioProcessor&);
    ~ARKAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    ARKAudioProcessor& audioProcessor;

    // Wrapper component — all UI children live here so setTransform() can
    // scale the entire fixed-1400px layout when the window size changes.
    juce::Component contentComponent;

    juce::MidiKeyboardComponent midiKeyboard;

    // Look and Feel instances
    KeyboardLookAndFeel   keyboardLookAndFeel;
    KnobLookAndFeel       knobLookAndFeel;
    WaveBtnLookAndFeel    waveBtnLookAndFeel;
    MasterDialLookAndFeel masterDialLookAndFeel;

    // OSC A Controls
    juce::Slider oscAOctave, oscASemitone, oscAFine;
    juce::Slider oscAUnison, oscADetune, oscABlend;
    juce::Slider oscAWtPos, oscAPan, oscALevel;
    juce::Slider oscAPhase, oscASpread, oscAChaos;

    juce::Label oscAOctaveLabel, oscASemitoneLabel, oscAFineLabel;
    juce::Label oscAUnisonLabel, oscADetuneLabel, oscABlendLabel;
    juce::Label oscAWtPosLabel, oscAPanLabel, oscALevelLabel;
    juce::Label oscAPhaseLabel, oscASpreadLabel, oscAChaosLabel;

    juce::TextButton oscASineBtn, oscASawBtn, oscASquareBtn, oscATriBtn;
    juce::TextButton oscAPowerBtn;

    // OSC A Preset controls
    juce::TextButton oscASaveBtn, oscAPrevBtn, oscANextBtn;
    juce::String oscAPresetName = "---";

    // OSC B Controls
    juce::Slider oscBOctave, oscBSemitone, oscBFine;
    juce::Slider oscBUnison, oscBDetune, oscBBlend;
    juce::Slider oscBWtPos, oscBPan, oscBLevel;
    juce::Slider oscBPhase, oscBSpread, oscBChaos;

    juce::Label oscBOctaveLabel, oscBSemitoneLabel, oscBFineLabel;
    juce::Label oscBUnisonLabel, oscBDetuneLabel, oscBBlendLabel;
    juce::Label oscBWtPosLabel, oscBPanLabel, oscBLevelLabel;
    juce::Label oscBPhaseLabel, oscBSpreadLabel, oscBChaosLabel;

    juce::TextButton oscBSineBtn, oscBSawBtn, oscBSquareBtn, oscBTriBtn;
    juce::TextButton oscBPowerBtn;

    // OSC B Preset controls
    juce::TextButton oscBSaveBtn, oscBPrevBtn, oscBNextBtn;
    juce::String oscBPresetName = "---";

    // Current waveform tracking
    int oscACurrentWave = 0;
    int oscBCurrentWave = 1;

    // APVTS parameter attachments for waveform selection
    std::unique_ptr<juce::ParameterAttachment> oscAWaveAttachment;
    std::unique_ptr<juce::ParameterAttachment> oscBWaveAttachment;

    // APVTS parameter attachments for power buttons
    std::unique_ptr<juce::ParameterAttachment> oscAPowerAttachment;
    std::unique_ptr<juce::ParameterAttachment> oscBPowerAttachment;

    // APVTS parameter attachments for OSC A knobs
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscAOctaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscASemitoneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscAFineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscAUnisonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscADetuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscABlendAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscAWtPosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscAPanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscALevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscAPhaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscASpreadAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscAChaosAttachment;

    // APVTS parameter attachments for OSC B knobs
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBOctaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBSemitoneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBFineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBUnisonAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBDetuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBBlendAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBWtPosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBPanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBPhaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBSpreadAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscBChaosAttachment;

    // -------------------------------------------------------------------------
    // Filter Controls
    // -------------------------------------------------------------------------
    juce::Slider filterCutoff, filterResonance, filterDrive;
    juce::Slider filterEnvAmount, filterLfoAmount;
    juce::Label  filterCutoffLabel, filterResonanceLabel, filterDriveLabel;
    juce::Label  filterEnvAmountLabel, filterLfoAmountLabel;

    juce::TextButton filterLpBtn, filterHpBtn, filterBpBtn, filterNotchBtn;
    int filterCurrentType = 0;
    int filterCharacter   = 0;
    juce::ComboBox filterCharacterBox;

    juce::Slider filterAttack, filterDecay, filterSustain, filterRelease;
    juce::Label  filterAttackLabel, filterDecayLabel, filterSustainLabel, filterReleaseLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterDriveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> filterCharacterAttachment;
    std::unique_ptr<juce::ParameterAttachment> filterModeAttachment;

    // -------------------------------------------------------------------------
    // SUB Oscillator
    // -------------------------------------------------------------------------
    juce::Slider subOctave, subLevel, subPan;
    juce::Label  subOctaveLabel, subLevelLabel, subPanLabel;

    juce::TextButton subSineBtn, subSquareBtn;
    juce::TextButton subPowerBtn;
    int subCurrentWave = 0;

    std::unique_ptr<juce::ParameterAttachment> subPowerAttachment;
    std::unique_ptr<juce::ParameterAttachment> subWaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subOctaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> subPanAttachment;

    // -------------------------------------------------------------------------
    // NOISE
    // -------------------------------------------------------------------------
    juce::Slider noiseLevel, noisePan, noiseCutoff, noiseLfoAmount;
    juce::Label  noiseLevelLabel, noisePanLabel, noiseCutoffLabel, noiseLfoAmountLabel;

    juce::TextButton noiseWhiteBtn, noisePinkBtn, noiseBrownBtn;
    juce::TextButton noisePowerBtn;
    juce::TextButton noiseTglBtn;
    bool noiseIsFree    = true;
    int  noiseCurrentType = 0;

    std::unique_ptr<juce::ParameterAttachment> noisePowerAttachment;
    std::unique_ptr<juce::ParameterAttachment> noiseTypeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseLevelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noisePanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseLfoAmountAttachment;

    // -------------------------------------------------------------------------
    // CONTROLS
    // -------------------------------------------------------------------------
    juce::TextButton voicingMonoBtn, voicingPolyBtn;
    juce::TextButton legatoBtn;
    juce::Slider maxVoices, minVoices, voiceSteal;
    juce::Label maxVoicesLabel, minVoicesLabel, voiceStealLabel;

    juce::TextButton veloTab, noteTab;
    int currentControlTab = 1;

    juce::Slider portaSlider, curveSlider;
    juce::Label portaLabel, curveLabel;
    juce::TextButton alwaysBtn, scaledBtn;

    bool isMonoMode      = false;
    bool isLegatoEnabled = false;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> portaAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> curveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> maxVoicesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> voiceStealAttachment;

    // -------------------------------------------------------------------------
    // ENV Section
    // -------------------------------------------------------------------------
    juce::TextButton env1Tab, env2Tab, env3Tab;
    int currentEnvTab = 0;

    juce::Slider env1Attack, env1Decay, env1Sustain, env1Release;
    juce::Label env1AttackLabel, env1DecayLabel, env1SustainLabel, env1ReleaseLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1AttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1DecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1SustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> env1ReleaseAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterLfoAmountAttachment;

    juce::ComboBox env3DestinationBox;
    juce::Label    env3DestinationLabel;
    juce::Slider   env3Attack, env3Decay, env3Sustain, env3Release, env3Amount;
    juce::Label    env3AttackLabel, env3DecayLabel, env3SustainLabel, env3ReleaseLabel, env3AmountLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> env3DestinationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   env3AttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   env3DecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   env3SustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   env3ReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   env3AmountAttachment;

    juce::TextButton envTrigBtn, envEnvBtn, envOffBtn;

    // -------------------------------------------------------------------------
    // LFO Section
    // -------------------------------------------------------------------------
    juce::TextButton lfo1Tab, lfo2Tab, lfo3Tab, lfo4Tab;
    int currentLfoTab = 0;

    juce::TextButton lfoSineBtn, lfoSawBtn, lfoSquareBtn, lfoTriBtn;
    int lfoCurrentWave = 0;

    juce::Slider lfo1Rate, lfo1Rise, lfo1Delay, lfo1Smooth;
    juce::Label  lfo1RateLabel, lfo1RiseLabel, lfo1DelayLabel, lfo1SmoothLabel;

    juce::Slider lfo2Rate, lfo2Rise, lfo2Delay, lfo2Smooth;
    juce::Label  lfo2RateLabel, lfo2RiseLabel, lfo2DelayLabel, lfo2SmoothLabel;

    juce::Slider lfo3Rate, lfo3Rise, lfo3Delay, lfo3Smooth;
    juce::Label  lfo3RateLabel, lfo3RiseLabel, lfo3DelayLabel, lfo3SmoothLabel;

    juce::Slider lfo4Rate, lfo4Rise, lfo4Delay, lfo4Smooth;
    juce::Label  lfo4RateLabel, lfo4RiseLabel, lfo4DelayLabel, lfo4SmoothLabel;

    std::unique_ptr<juce::ParameterAttachment> lfo1WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1RiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1DelayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1SmoothAttachment;

    std::unique_ptr<juce::ParameterAttachment> lfo2WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo2RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo2RiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo2DelayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo2SmoothAttachment;

    std::unique_ptr<juce::ParameterAttachment> lfo3WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo3RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo3RiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo3DelayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo3SmoothAttachment;

    std::unique_ptr<juce::ParameterAttachment> lfo4WaveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo4RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo4RiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo4DelayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo4SmoothAttachment;

    juce::TextButton bpmBtn;

    // -------------------------------------------------------------------------
    // ARP Section
    // -------------------------------------------------------------------------
    juce::TextButton arpOnOffBtn;
    juce::TextButton arpModeBtn;
    juce::Slider     arpRateKnob, arpGateKnob, arpSwingKnob, arpOctaveKnob;
    juce::Label      arpRateLabel, arpGateLabel, arpSwingLabel, arpOctaveLabel;
    juce::TextButton arpUpBtn, arpDownBtn, arpUpDownBtn, arpRandomBtn, arpOrderBtn;
    juce::TextButton arpLatchBtn;
    bool isArpOn   = false;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> arpRateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> arpGateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> arpSwingAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> arpOctaveAttachment;

    // -------------------------------------------------------------------------
    // MOD Section
    // -------------------------------------------------------------------------
    juce::Slider modFattnessKnob, modHpFilterKnob, modFmKnob, modTightenKnob, modHumanKnob;
    juce::Label  modFattnessLabel, modHpFilterLabel, modFmLabel, modTightenLabel, modHumanLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modFattnessAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modHpFilterAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modFmAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modTightenAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modHumanAttachment;

    // -------------------------------------------------------------------------
    // MASTER Section
    // -------------------------------------------------------------------------
    juce::Slider masterVolumeDial, masterTuneDial, masterPanDial, masterTranspDial;
    juce::Label  masterVolumeLabel, masterTuneLabel, masterPanLabel, masterTranspLabel;
    juce::Label  masterVolumeValue, masterTuneValue, masterPanValue, masterTranspValue;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterVolumeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterTuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterPanAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterTranspAttachment;

    void drawEnvelopeCurve (juce::Graphics& g, int x, int y, int width, int height,
                            float attack, float decay, float sustain, float release);
    void drawLfoCurve      (juce::Graphics& g, int x, int y, int width, int height, int lfoType);
    void drawVelocityCurve (juce::Graphics& g, int x, int y, int width, int height, float curve);
    void drawNoteCurve     (juce::Graphics& g, int x, int y, int width, int height, float curve);

    void setupKnob (juce::Slider& slider, juce::Label& label,
                    const juce::String& labelText,
                    double min, double max, double interval, double defaultValue);
    void setupFilterButton (juce::TextButton& button, const juce::String& text, int typeIndex);
    void setupWaveButton   (juce::TextButton& button, const juce::String& text);
    void drawWaveform         (juce::Graphics& g, int x, int y, int width, int height, int waveType);
    void drawWaveformVertical (juce::Graphics& g, int x, int y, int width, int height, int waveType);
    void drawNoiseVisualization (juce::Graphics& g, int x, int y, int width, int height, int noiseType);
    void drawFilterResponse (juce::Graphics& g, int x, int y, int width, int height,
                             double cutoff, double resonance, int filterType);

    void styleGlobalCombo  (juce::ComboBox& box);
    void styleGlobalSlider (juce::Slider&   slider);

    // -------------------------------------------------------------------------
    // GLOBAL Page
    // -------------------------------------------------------------------------
    juce::ComboBox globalVoiceModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> globalVoiceModeAttachment;

    juce::ComboBox globalLegatoBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> globalLegatoAttachment;

    juce::Slider globalMaxVoicesSlider;
    juce::Label  globalMaxVoicesLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalMaxVoicesAttachment;

    juce::ComboBox globalVoiceStealBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> globalVoiceStealAttachment;

    juce::Slider globalPitchBendUpSlider, globalPitchBendDownSlider;
    juce::Label  globalPitchBendUpLabel,  globalPitchBendDownLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalPitchBendUpAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalPitchBendDownAttachment;

    juce::Slider   globalPortaTimeSlider, globalPortaCurveSlider;
    juce::Label    globalPortaTimeLabel,  globalPortaCurveLabel;
    juce::ComboBox globalPortaModeBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   globalPortaTimeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   globalPortaCurveAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> globalPortaModeAttachment;

    juce::Slider globalVeloCurveSlider;
    juce::Label  globalVeloCurveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalVeloCurveAttachment;

    juce::Slider globalNoteTrackSlider;
    juce::Label  globalNoteTrackLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalNoteTrackAttachment;

    juce::ComboBox globalModWheelDestBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> globalModWheelDestAttachment;

    juce::ComboBox globalMidiChannelBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> globalMidiChannelAttachment;

    juce::ComboBox globalMidiThruBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> globalMidiThruAttachment;

    int  globalWindowSize   = 1;
    bool globalShowKeyboard = true;
    bool globalShowScope    = true;

    juce::ComboBox globalWindowSizeBox;
    juce::ComboBox globalShowKeyboardBox;
    juce::ComboBox globalShowScopeBox;

    // -------------------------------------------------------------------------
    // MODULATION MATRIX — 16 slots
    // -------------------------------------------------------------------------
    juce::ComboBox   matrixSourceBox[16];
    juce::ComboBox   matrixDestBox[16];
    juce::Slider     matrixAmountSlider[16];
    juce::TextButton matrixEnableBtn[16];

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> matrixSourceAttachment[16];
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> matrixDestAttachment[16];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>   matrixAmountAttachment[16];

    // -------------------------------------------------------------------------
    // Page Navigation
    // -------------------------------------------------------------------------
    juce::TextButton oscTabBtn, fxTabBtn, matrixTabBtn, globalTabBtn;
    int currentPage = 0;
    void showPage (int pageIndex);

    // -------------------------------------------------------------------------
    // Preset System (header bar)
    // -------------------------------------------------------------------------
    juce::TextButton presetBtn;      // shows current preset name, opens browser on click
    juce::TextButton savePresetBtn;  // opens the save dialog

    juce::String currentPresetName   = "Init Preset";
    juce::String currentPresetFolder = "User Presets";

    std::unique_ptr<PresetBrowserOverlay> presetOverlay;

    // Opens the preset browser overlay (browse / load)
    void openPresetBrowser();
    void closePresetBrowser();

    // Opens the save dialog (name + folder selector) — wired to savePresetBtn
    void openSavePresetDialog();

    // -------------------------------------------------------------------------
    // PIANO CONTROLS
    // -------------------------------------------------------------------------
    juce::Slider pitchWheelSlider;
    juce::Slider modWheelSlider;

    juce::TextButton octMinusBtn, octPlusBtn;
    juce::TextButton semiMinusBtn, semiPlusBtn;

    juce::Label octValueLabel;
    juce::Label semiValueLabel;

    int keyboardOctave = 0;
    int keyboardSemi   = 0;

    void updateOctSemiDisplay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ARKAudioProcessorEditor)
};
