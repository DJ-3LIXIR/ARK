#include "FXPage.h"

// =============================================================================
// RACK LIST COMPONENT
// =============================================================================

RackListComponent::RackListComponent (InvisibleButtonLookAndFeel& lnf)
    : invisibleButtonLnf (lnf)
{
    // Select All button
    selectAllBtn.setLookAndFeel (&invisibleButtonLnf);
    selectAllBtn.setClickingTogglesState (true);
    selectAllBtn.onClick = [this] { getParentComponent()->repaint(); repaint(); };
    addAndMakeVisible (selectAllBtn);

    // Add Rack button
    addRackBtn.setLookAndFeel (&invisibleButtonLnf);
    addRackBtn.onClick = [this] { addRack(); };
    addAndMakeVisible (addRackBtn);

    // Start with 3 racks
    for (int i = 0; i < 3; ++i)
        addRack();
}

RackListComponent::~RackListComponent()
{
    selectAllBtn.setLookAndFeel (nullptr);
    addRackBtn.setLookAndFeel (nullptr);
    for (auto* slot : racks)
    {
        slot->selectButton.setLookAndFeel (nullptr);
        slot->pluginSelector.setLookAndFeel (nullptr);
        slot->powerButton.setLookAndFeel (nullptr);
        slot->closeButton.setLookAndFeel (nullptr);
        slot->muteButton.setLookAndFeel (nullptr);
        slot->soloButton.setLookAndFeel (nullptr);
    }
}

void RackListComponent::selectRack (int index)
{
    if (index >= 0 && index < (int) racks.size() && index != selectedRackIndex)
    {
        selectedRackIndex = index;
        repaint();
        if (onSelectionChanged)
            onSelectionChanged();
    }
}

RackSlot* RackListComponent::getSelectedRack()
{
    if (selectedRackIndex >= 0 && selectedRackIndex < (int) racks.size())
        return racks[selectedRackIndex];
    return nullptr;
}

const RackSlot* RackListComponent::getSelectedRack() const
{
    if (selectedRackIndex >= 0 && selectedRackIndex < (int) racks.size())
        return racks[selectedRackIndex];
    return nullptr;
}

RackSlot* RackListComponent::getRack (int index)
{
    if (index >= 0 && index < (int) racks.size())
        return racks[index];
    return nullptr;
}

const RackSlot* RackListComponent::getRack (int index) const
{
    if (index >= 0 && index < (int) racks.size())
        return racks[index];
    return nullptr;
}

void RackListComponent::setupRackSlot (RackSlot* slot, int index)
{
    slot->selectButton.setLookAndFeel (&invisibleButtonLnf);
    slot->selectButton.onClick = [this, index] { selectRack (index); };
    addAndMakeVisible (slot->selectButton);

    slot->pluginSelector.setLookAndFeel (&invisibleButtonLnf);
    slot->pluginSelector.onClick = [this, index] {
        selectRack (index);
        onPluginSelectorClicked (index);
    };
    addAndMakeVisible (slot->pluginSelector);

    slot->powerButton.setLookAndFeel (&invisibleButtonLnf);
    slot->powerButton.setClickingTogglesState (true);
    slot->powerButton.onClick = [this, index] { selectRack (index); repaint(); if (onRackStateChanged) onRackStateChanged(); };
    addAndMakeVisible (slot->powerButton);

    slot->closeButton.setLookAndFeel (&invisibleButtonLnf);
    slot->closeButton.onClick = [this, index] { onCloseClicked (index); };
    addAndMakeVisible (slot->closeButton);

    slot->muteButton.setLookAndFeel (&invisibleButtonLnf);
    slot->muteButton.setClickingTogglesState (true);
    slot->muteButton.onClick = [this, index] { selectRack (index); repaint(); if (onRackStateChanged) onRackStateChanged(); };
    addAndMakeVisible (slot->muteButton);

    slot->soloButton.setLookAndFeel (&invisibleButtonLnf);
    slot->soloButton.setClickingTogglesState (true);
    slot->soloButton.onClick = [this, index] { selectRack (index); repaint(); };
    addAndMakeVisible (slot->soloButton);
}

void RackListComponent::addRack()
{
    auto* slot = new RackSlot();
    int index = (int) racks.size();
    setupRackSlot (slot, index);
    racks.add (slot);

    setSize (getWidth(), getContentHeight());
    resized();
    repaint();

    if (onRackStateChanged)
        onRackStateChanged();

    // Scroll to bottom if inside a viewport
    if (auto* vp = findParentComponentOfClass<juce::Viewport>())
        vp->setViewPosition (0, juce::jmax (0, getHeight() - vp->getHeight()));
}

void RackListComponent::removeRack (int index)
{
    if (index < 0 || index >= (int) racks.size() || racks.size() <= 1)
        return;

    // Clean up LookAndFeel before removing
    auto* slot = racks[index];
    slot->selectButton.setLookAndFeel (nullptr);
    slot->pluginSelector.setLookAndFeel (nullptr);
    slot->powerButton.setLookAndFeel (nullptr);
    slot->closeButton.setLookAndFeel (nullptr);
    slot->muteButton.setLookAndFeel (nullptr);
    slot->soloButton.setLookAndFeel (nullptr);

    removeChildComponent (&slot->selectButton);
    removeChildComponent (&slot->pluginSelector);
    removeChildComponent (&slot->powerButton);
    removeChildComponent (&slot->closeButton);
    removeChildComponent (&slot->muteButton);
    removeChildComponent (&slot->soloButton);

    racks.remove (index);

    // Re-wire click handlers with correct indices
    for (int i = 0; i < (int) racks.size(); ++i)
    {
        racks[i]->selectButton.onClick = [this, i] { selectRack (i); };
        racks[i]->pluginSelector.onClick = [this, i] { selectRack (i); onPluginSelectorClicked (i); };
        racks[i]->closeButton.onClick = [this, i] { onCloseClicked (i); };
        racks[i]->powerButton.onClick = [this, i] { selectRack (i); repaint(); if (onRackStateChanged) onRackStateChanged(); };
        racks[i]->muteButton.onClick = [this, i] { selectRack (i); repaint(); if (onRackStateChanged) onRackStateChanged(); };
        racks[i]->soloButton.onClick = [this, i] { selectRack (i); repaint(); };
    }

    // Adjust selection
    if (selectedRackIndex >= (int) racks.size())
        selectedRackIndex = (int) racks.size() - 1;

    setSize (getWidth(), getContentHeight());
    resized();
    repaint();

    if (onRackStateChanged)
        onRackStateChanged();
    if (onSelectionChanged)
        onSelectionChanged();
}

float RackListComponent::getSlotStartX() const { return RACK_NUMBER_W + 8.0f; }
float RackListComponent::getSlotMaxW() const { return (float) getWidth() - RACK_NUMBER_W - 16.0f; }
float RackListComponent::getFirstSlotY() const { return HEADER_H; }

int RackListComponent::getContentHeight() const
{
    return (int)(HEADER_H + (float) racks.size() * SLOT_SPACING + 40.0f);  // 40 for add button
}

juce::String RackListComponent::getPluginName (int pluginId) const
{
    switch (pluginId)
    {
        case 1:  return "Hades";
        case 2:  return "Apollo";
        case 3:  return "Orion";
        default: return "No Audio Effects";
    }
}

void RackListComponent::onPluginSelectorClicked (int rackIndex)
{
    if (rackIndex < 0 || rackIndex >= (int) racks.size()) return;

    auto* slot = racks[rackIndex];
    juce::PopupMenu menu;
    menu.addItem (1, "No Audio Effects", true, slot->selectedPlugin == 0);
    menu.addSeparator();
    menu.addItem (2, "Hades",  true, slot->selectedPlugin == 1);
    menu.addItem (3, "Apollo", true, slot->selectedPlugin == 2);
    menu.addItem (4, "Orion",  true, slot->selectedPlugin == 3);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&slot->pluginSelector),
        [this, rackIndex] (int result)
        {
            if (rackIndex >= (int) racks.size()) return;
            auto* s = racks[rackIndex];
            int newPlugin = s->selectedPlugin;
            if (result == 1) newPlugin = 0;
            else if (result == 2) newPlugin = 1;
            else if (result == 3) newPlugin = 2;
            else if (result == 4) newPlugin = 3;

            if (result > 0 && newPlugin != s->selectedPlugin)
            {
                s->selectedPlugin = newPlugin;
                repaint();
                if (onPluginChanged)
                    onPluginChanged (rackIndex);
            }
        });
}

void RackListComponent::onCloseClicked (int rackIndex)
{
    removeRack (rackIndex);
}

// =============================================================================
// RACK LIST — PAINT
// =============================================================================
void RackListComponent::paint (juce::Graphics& g)
{
    auto gold     = juce::Colour (0xffD4A017);
    auto goldDim  = juce::Colour (0xff64470A);

    // Black background
    g.fillAll (juce::Colours::black);

    // RACK label
    g.setColour (gold);
    g.setFont (juce::Font (juce::FontOptions (18.0f).withStyle ("Bold")));
    g.drawText ("RACK", 20, 8, (int) getWidth() - 50, 32, juce::Justification::centredLeft, true);

    // S button — OFF: black bg, gold text | ON: gold bg, black text
    const float sSize = 32.0f;
    const float sX = (float) getWidth() - sSize - 4.0f;
    const float sY = 8.0f;
    bool sAllOn = selectAllBtn.getToggleState();
    g.setColour (sAllOn ? gold : juce::Colours::black);
    g.fillRoundedRectangle (sX, sY, sSize, sSize, 4.0f);
    g.setColour (goldDim);
    g.drawRoundedRectangle (sX, sY, sSize, sSize, 4.0f, 1.0f);
    g.setColour (sAllOn ? juce::Colours::black : gold);
    g.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
    g.drawText ("S", (int)sX, (int)sY, (int)sSize, (int)sSize, juce::Justification::centred, true);

    // Divider line
    g.setColour (gold);
    g.drawLine (8.0f, HEADER_H - 4.0f, (float) getWidth() - 8.0f, HEADER_H - 4.0f, 1.5f);

    // Draw all rack slots
    for (int i = 0; i < (int) racks.size(); ++i)
    {
        float slotY = getFirstSlotY() + (float)i * SLOT_SPACING;
        drawRackSlot (g, i, slotY);
    }

    // "Add Another Rack" button area
    float addY = getFirstSlotY() + (float) racks.size() * SLOT_SPACING;
    float addX = getSlotStartX();
    float addW = getSlotMaxW();
    g.setColour (goldDim);
    g.drawRoundedRectangle (addX, addY, addW, 30.0f, 4.0f, 1.0f);
    g.setColour (gold.withAlpha (0.6f));
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.drawText ("+ Add Another Rack", (int)addX, (int)addY, (int)addW, 30,
               juce::Justification::centred, true);
}

void RackListComponent::drawRackSlot (juce::Graphics& g, int index, float slotY)
{
    auto gold     = juce::Colour (0xffD4A017);
    auto goldDim  = juce::Colour (0xff64470A);
    auto ctrlBg   = juce::Colour (0xffC49010);

    float slotX = getSlotStartX();
    float slotW = getSlotMaxW();

    // ── Selection glow ──
    bool isSelected = (index == selectedRackIndex);
    if (isSelected)
    {
        // Draw a golden glow behind the selected rack
        auto glowColour = gold.withAlpha (0.25f);
        for (int glow = 3; glow >= 1; --glow)
        {
            float expand = (float) glow * 2.0f;
            g.setColour (glowColour.withMultipliedAlpha (1.0f / (float)glow));
            g.fillRoundedRectangle (slotX - expand, slotY - expand,
                                    slotW + expand * 2.0f, SLOT_H + expand * 2.0f, 6.0f);
        }
    }

    // Rack number label
    g.setColour (isSelected ? gold : gold.withAlpha (0.7f));
    g.setFont (juce::Font (juce::FontOptions (9.0f).withStyle ("Bold")));
    g.drawText (juce::String (index + 1), 4, (int)slotY, (int)RACK_NUMBER_W, (int)SLOT_H,
               juce::Justification::centred, true);

    // Golden slot background
    g.setColour (gold);
    g.fillRoundedRectangle (slotX, slotY, slotW, SLOT_H, 4.0f);

    // Selected rack gets a brighter border
    if (isSelected)
    {
        g.setColour (juce::Colours::white.withAlpha (0.6f));
        g.drawRoundedRectangle (slotX, slotY, slotW, SLOT_H, 4.0f, 2.0f);
    }
    else
    {
        g.setColour (goldDim);
        g.drawRoundedRectangle (slotX, slotY, slotW, SLOT_H, 4.0f, 1.0f);
    }

    // Plugin selector dropdown (left 70%)
    float ddX = slotX + 4.0f;
    float ddY = slotY + 4.0f;
    float ddW = slotW * 0.70f;
    float ddH = SLOT_H - 8.0f;

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (ddX, ddY, ddW, ddH, 3.0f);
    g.setColour (gold.withAlpha (0.5f));
    g.drawRoundedRectangle (ddX, ddY, ddW, ddH, 3.0f, 1.0f);
    g.setColour (gold);
    g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText (getPluginName (racks[index]->selectedPlugin), (int)ddX, (int)ddY, (int)ddW, (int)ddH,
               juce::Justification::centred, true);

    // Controls area (right 30%)
    float ctrlX = ddX + ddW + 4.0f;
    float ctrlY = slotY + 4.0f;
    float ctrlW = slotW - ddW - 8.0f;
    float ctrlH = SLOT_H - 8.0f;

    g.setColour (ctrlBg);
    g.fillRoundedRectangle (ctrlX, ctrlY, ctrlW, ctrlH, 3.0f);

    float powerH = ctrlH * 0.5f;
    float muteY = ctrlY + powerH;

    // Power button (top — left 2/3) — OFF: black bg, gold text | ON: gold bg, black text
    float pwrW = ctrlW * 0.667f;
    float closeW = ctrlW - pwrW;

    bool pwrOn = racks[index]->powerButton.getToggleState();
    g.setColour (pwrOn ? gold : juce::Colours::black);
    g.fillRect (ctrlX, ctrlY, pwrW, powerH);
    g.setColour (pwrOn ? juce::Colours::black : gold);
    g.setFont (juce::Font (juce::FontOptions (10.0f).withStyle ("Bold")));
    g.drawText ("PWR", (int)ctrlX, (int)ctrlY, (int)pwrW, (int)powerH,
               juce::Justification::centred, true);

    // Close button (top — right 1/3)
    g.setColour (juce::Colour (0xff8B2500));
    g.fillRect (ctrlX + pwrW, ctrlY, closeW, powerH);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("X", (int)(ctrlX + pwrW), (int)ctrlY, (int)closeW, (int)powerH,
               juce::Justification::centred, true);

    // Vertical divider between PWR and X
    g.setColour (goldDim);
    g.drawLine (ctrlX + pwrW, ctrlY, ctrlX + pwrW, ctrlY + powerH, 1.0f);

    // Horizontal divider
    g.setColour (goldDim);
    g.drawLine (ctrlX, muteY, ctrlX + ctrlW, muteY, 1.0f);

    // Mute (bottom left half) — OFF: black bg, gold text | ON: gold bg, black text
    float muteW = ctrlW * 0.5f;
    bool muteOn = racks[index]->muteButton.getToggleState();
    g.setColour (muteOn ? gold : juce::Colours::black);
    g.fillRect (ctrlX, muteY, muteW, powerH);
    g.setColour (muteOn ? juce::Colours::black : gold);
    g.setFont (juce::Font (juce::FontOptions (11.0f).withStyle ("Bold")));
    g.drawText ("M", (int)ctrlX, (int)muteY, (int)muteW, (int)powerH,
               juce::Justification::centred, true);

    // Vertical divider between M and S
    g.setColour (goldDim);
    g.drawLine (ctrlX + muteW, muteY, ctrlX + muteW, muteY + powerH, 1.0f);

    // Solo (bottom right half) — OFF: black bg, gold text | ON: gold bg, black text
    bool soloOn = racks[index]->soloButton.getToggleState();
    g.setColour (soloOn ? gold : juce::Colours::black);
    g.fillRect (ctrlX + muteW, muteY, muteW, powerH);
    g.setColour (soloOn ? juce::Colours::black : gold);
    g.drawText ("S", (int)(ctrlX + muteW), (int)muteY, (int)muteW, (int)powerH,
               juce::Justification::centred, true);
}

// =============================================================================
// RACK LIST — RESIZED
// =============================================================================
void RackListComponent::resized()
{
    float slotX = getSlotStartX();
    float slotW = getSlotMaxW();

    for (int i = 0; i < (int) racks.size(); ++i)
    {
        float slotY = getFirstSlotY() + (float)i * SLOT_SPACING;
        auto* slot = racks[i];

        // Selection hit area — covers rack number + gap (from x=0 to start of slot)
        slot->selectButton.setBounds (0, (int)slotY, (int)slotX, (int)SLOT_H);

        // Plugin selector (left 70%)
        float ddX = slotX + 4.0f;
        float ddY = slotY + 4.0f;
        float ddW = slotW * 0.70f;
        float ddH = SLOT_H - 8.0f;
        slot->pluginSelector.setBounds ((int)ddX, (int)ddY, (int)ddW, (int)ddH);

        // Controls area
        float ctrlX = ddX + ddW + 4.0f;
        float ctrlY = slotY + 4.0f;
        float ctrlW = slotW - ddW - 8.0f;
        float ctrlH = SLOT_H - 8.0f;
        float powerH = ctrlH * 0.5f;

        // Power (top left 2/3)
        float pwrW = ctrlW * 0.667f;
        float closeW = ctrlW - pwrW;
        slot->powerButton.setBounds ((int)ctrlX, (int)ctrlY, (int)pwrW, (int)powerH);

        // Close (top right 1/3)
        slot->closeButton.setBounds ((int)(ctrlX + pwrW), (int)ctrlY, (int)closeW, (int)powerH);

        // Mute and Solo (bottom half)
        float muteY = ctrlY + powerH;
        float muteW = ctrlW * 0.5f;
        slot->muteButton.setBounds ((int)ctrlX, (int)muteY, (int)muteW, (int)powerH);
        slot->soloButton.setBounds ((int)(ctrlX + muteW), (int)muteY, (int)muteW, (int)powerH);
    }

    // Select All button
    const float sSize = 32.0f;
    selectAllBtn.setBounds ((int)((float)getWidth() - sSize - 4.0f), 8, (int)sSize, (int)sSize);

    // Add Rack button
    float addY = getFirstSlotY() + (float) racks.size() * SLOT_SPACING;
    addRackBtn.setBounds ((int) getSlotStartX(), (int)addY, (int) getSlotMaxW(), 30);
}

// =============================================================================
// FX PAGE
// =============================================================================

FXPage::FXPage (ARKAudioProcessor* processor)
    : processor (processor), rackList (invisibleButtonLnf)
{
    // Viewport for scrollable rack list
    rackViewport.setViewedComponent (&rackList, false);
    rackViewport.setScrollBarsShown (true, false);  // vertical scroll only
    rackViewport.getVerticalScrollBar().setColour (juce::ScrollBar::thumbColourId, juce::Colour (0xffD4A017));
    addAndMakeVisible (rackViewport);

    // Wire callbacks from rack list
    rackList.onSelectionChanged = [this] { updateDisplayPanel(); };
    rackList.onPluginChanged = [this] (int rackIndex) {
        auto* slot = rackList.getRack (rackIndex);
        if (slot != nullptr)
            createPluginForRack (rackIndex, slot->selectedPlugin);
    };
    rackList.onRackStateChanged = [this] { syncFxChainToProcessor(); };
}

FXPage::~FXPage()
{
    // Clear FX chain in processor before destroying plugin instances
    if (processor != nullptr)
    {
        juce::SpinLock::ScopedLockType lock (processor->fxChainLock);
        for (int i = 0; i < ARKAudioProcessor::MAX_FX_SLOTS; ++i)
            processor->fxChain[i] = {};
        processor->numFxSlots.store (0);
    }
    currentEditor.reset();
}

void FXPage::createPluginForRack (int rackIndex, int pluginId)
{
    auto* slot = rackList.getRack (rackIndex);
    if (slot == nullptr) return;

    // Remove old processor from FX chain before destroying it
    if (processor != nullptr)
    {
        juce::SpinLock::ScopedLockType lock (processor->fxChainLock);
        if (rackIndex < ARKAudioProcessor::MAX_FX_SLOTS)
            processor->fxChain[rackIndex].processor = nullptr;
    }

    // Destroy existing processor
    slot->pluginProcessor.reset();

    // Create new processor based on plugin ID
    switch (pluginId)
    {
        case 1: slot->pluginProcessor = std::make_unique<HadesAudioProcessor>(); break;
        case 2: slot->pluginProcessor = std::make_unique<APOLLOAudioProcessor>(); break;
        case 3: slot->pluginProcessor = std::make_unique<OrionSoundEQAudioProcessor>(); break;
        default: break;  // No Audio Effects — no processor
    }

    // Prepare the processor with ARK's audio settings
    if (slot->pluginProcessor != nullptr && processor != nullptr)
    {
        slot->pluginProcessor->setPlayConfigDetails (
            2, 2,  // stereo in/out
            processor->getSampleRate(),
            processor->getBlockSize());
        slot->pluginProcessor->prepareToPlay (
            processor->getSampleRate(),
            processor->getBlockSize());
    }

    // Sync the full chain to the processor
    syncFxChainToProcessor();

    // Update display if this is the selected rack
    if (rackIndex == rackList.selectedRackIndex)
        updateDisplayPanel();
}

void FXPage::updateDisplayPanel()
{
    // Remove current editor
    if (currentEditor != nullptr)
    {
        removeChildComponent (currentEditor.get());
        currentEditor.reset();
    }

    auto* slot = rackList.getSelectedRack();
    if (slot == nullptr || slot->pluginProcessor == nullptr)
    {
        repaint();
        return;
    }

    // Create the editor from the processor
    if (slot->pluginProcessor->hasEditor())
    {
        currentEditor.reset (slot->pluginProcessor->createEditor());

        if (currentEditor != nullptr)
        {
            addAndMakeVisible (currentEditor.get());

            // Scale editor to fit in the display bounds
            if (! displayBounds.isEmpty())
            {
                int edW = currentEditor->getWidth();
                int edH = currentEditor->getHeight();

                if (edW > 0 && edH > 0)
                {
                    float scaleX = (float) displayBounds.getWidth() / (float) edW;
                    float scaleY = (float) displayBounds.getHeight() / (float) edH;
                    float scale = juce::jmin (scaleX, scaleY);

                    int scaledW = (int)((float) edW * scale);
                    int scaledH = (int)((float) edH * scale);
                    int centreX = displayBounds.getX() + (displayBounds.getWidth() - scaledW) / 2;
                    int centreY = displayBounds.getY() + (displayBounds.getHeight() - scaledH) / 2;

                    currentEditor->setTransform (juce::AffineTransform::scale (scale));
                    currentEditor->setTopLeftPosition ((int)((float) centreX / scale),
                                                       (int)((float) centreY / scale));
                }
            }
        }
    }

    repaint();
}

void FXPage::paint (juce::Graphics& g)
{
    auto gold     = juce::Colour (0xffD4A017);
    auto goldDim  = juce::Colour (0xff64470A);
    auto bgPanel  = juce::Colour (0xff1e1e1e);

    const float borderWidth = 1.5f;

    // =====================================================================
    // LAYOUT — full height, gold top line then content below
    // =====================================================================
    const float panelW = (float) getWidth();
    const float contentY = borderWidth;   // panels start just below the gold top line
    const float contentH = (float) getHeight() - contentY;

    const float rackPanelW = 300.0f;
    const float displayPanelX = rackPanelW;
    const float displayPanelW = panelW - rackPanelW;

    // Gold top line
    g.setColour (gold);
    g.fillRect (0.0f, 0.0f, panelW, borderWidth);

    // Left black panel background (below gold line)
    g.setColour (juce::Colours::black);
    g.fillRect (0.0f, contentY, rackPanelW, contentH);

    // Right display panel (below gold line)
    g.setColour (bgPanel);
    g.fillRect (displayPanelX, contentY, displayPanelW, contentH);

    // Gold side borders (left and right edges)
    g.setColour (gold);
    g.fillRect (0.0f, 0.0f, borderWidth, (float) getHeight());
    g.fillRect (panelW - borderWidth, 0.0f, borderWidth, (float) getHeight());

    // Vertical divider
    g.setColour (goldDim);
    g.drawLine (rackPanelW, contentY, rackPanelW, contentY + contentH, 1.0f);

    // Display panel content
    if (currentEditor == nullptr)
    {
        // No editor — show placeholder text
        auto* selectedSlot = rackList.getSelectedRack();
        juce::String message = "Choose a Plugin";
        if (selectedSlot != nullptr && selectedSlot->selectedPlugin == 0)
            message = "No Audio Effects";

        g.setColour (gold.withAlpha (0.4f));
        g.setFont (juce::Font (juce::FontOptions (24.0f).withStyle ("Bold")));
        g.drawText (message, (int)displayPanelX, (int)contentY, (int)displayPanelW, (int)contentH,
                   juce::Justification::centred, true);
    }
}

void FXPage::paintOverChildren (juce::Graphics& g)
{
    // Redraw gold borders on top of child components so plugin editors
    // never protrude through the edges
    const float borderWidth = 1.5f;
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    g.setColour (juce::Colour (0xffD4A017));
    g.fillRect (0.0f, 0.0f, w, borderWidth);            // top
    g.fillRect (0.0f, 0.0f, borderWidth, h);             // left
    g.fillRect (w - borderWidth, 0.0f, borderWidth, h);  // right
}

void FXPage::resized()
{
    const float borderWidth = 1.5f;
    const float contentY = borderWidth;
    const float contentH = (float) getHeight() - contentY;
    const float rackPanelW = 300.0f;
    const float displayPanelX = rackPanelW;
    const float displayPanelW = (float) getWidth() - rackPanelW;

    // Viewport fills the left black panel area (below gold border)
    rackViewport.setBounds (0, (int)contentY, (int)rackPanelW, (int)contentH);

    // Rack list component width matches viewport, height is dynamic
    rackList.setSize ((int)rackPanelW - (rackViewport.isVerticalScrollBarShown() ? 10 : 0),
                      juce::jmax ((int)contentH, rackList.getContentHeight()));

    // Cache display bounds for editor positioning — fills entire display panel
    displayBounds = juce::Rectangle<int> ((int)displayPanelX,
                                           (int)contentY,
                                           (int)displayPanelW,
                                           (int)contentH);

    // Reposition current editor if it exists
    if (currentEditor != nullptr)
        updateDisplayPanel();
}

void FXPage::syncFxChainToProcessor()
{
    if (processor == nullptr) return;

    juce::SpinLock::ScopedLockType lock (processor->fxChainLock);

    int numRacks = rackList.getNumRacks();
    int slotCount = juce::jmin (numRacks, (int) ARKAudioProcessor::MAX_FX_SLOTS);

    for (int i = 0; i < slotCount; ++i)
    {
        auto* slot = rackList.getRack (i);
        if (slot != nullptr)
        {
            processor->fxChain[i].processor = slot->pluginProcessor.get();
            processor->fxChain[i].powered   = slot->powerButton.getToggleState();
            processor->fxChain[i].muted     = slot->muteButton.getToggleState();
        }
        else
        {
            processor->fxChain[i] = {};
        }
    }

    // Clear any remaining slots
    for (int i = slotCount; i < ARKAudioProcessor::MAX_FX_SLOTS; ++i)
        processor->fxChain[i] = {};

    processor->numFxSlots.store (slotCount);
}

std::unique_ptr<juce::XmlElement> FXPage::saveFXState() const
{
    auto fxStateXml = std::make_unique<juce::XmlElement> ("FXSTATE");

    int numRacks = rackList.getNumRacks();
    fxStateXml->setAttribute ("numRacks", numRacks);
    fxStateXml->setAttribute ("selectedRack", rackList.selectedRackIndex);

    // Save each rack's state
    for (int i = 0; i < numRacks; ++i)
    {
        auto* slot = rackList.getRack (i);
        if (slot == nullptr) continue;

        auto rackXml = std::make_unique<juce::XmlElement> ("RACK");
        rackXml->setAttribute ("index", i);
        rackXml->setAttribute ("pluginId", slot->selectedPlugin);
        rackXml->setAttribute ("powered", slot->powerButton.getToggleState());
        rackXml->setAttribute ("muted", slot->muteButton.getToggleState());

        // Save plugin's state (APVTS)
        if (slot->pluginProcessor != nullptr)
        {
            juce::MemoryBlock pluginStateData;
            slot->pluginProcessor->getStateInformation (pluginStateData);

            auto pluginStateXml = std::make_unique<juce::XmlElement> ("PLUGINSTATE");
            pluginStateXml->setAttribute ("data", pluginStateData.toBase64Encoding());
            rackXml->addChildElement (pluginStateXml.release());
        }

        fxStateXml->addChildElement (rackXml.release());
    }

    return fxStateXml;
}

void FXPage::restoreFXState (const juce::XmlElement* fxStateXml)
{
    if (fxStateXml == nullptr || ! fxStateXml->hasTagName ("FXSTATE"))
        return;

    int numRacks = fxStateXml->getIntAttribute ("numRacks", 0);
    int selectedRack = fxStateXml->getIntAttribute ("selectedRack", 0);

    // Clear existing racks
    while (rackList.getNumRacks() > 1)
        rackList.removeRack (rackList.getNumRacks() - 1);

    // Restore each rack
    forEachXmlChildElement (*fxStateXml, rackXml)
    {
        if (! rackXml->hasTagName ("RACK"))
            continue;

        int pluginId = rackXml->getIntAttribute ("pluginId", 0);
        bool powered = rackXml->getBoolAttribute ("powered", true);
        bool muted = rackXml->getBoolAttribute ("muted", false);

        // Add rack if needed
        int rackIndex = rackXml->getIntAttribute ("index", 0);
        while (rackList.getNumRacks() <= rackIndex)
            rackList.addRack();

        auto* slot = rackList.getRack (rackIndex);
        if (slot == nullptr) continue;

        // Create the plugin
        slot->selectedPlugin = pluginId;
        createPluginForRack (rackIndex, pluginId);

        // Restore toggle states
        slot->powerButton.setToggleState (powered, juce::dontSendNotification);
        slot->muteButton.setToggleState (muted, juce::dontSendNotification);

        // Restore plugin state
        auto* pluginStateXml = rackXml->getChildByName ("PLUGINSTATE");
        if (pluginStateXml != nullptr && slot->pluginProcessor != nullptr)
        {
            juce::String encodedData = pluginStateXml->getStringAttribute ("data", "");
            juce::MemoryBlock pluginStateData;
            pluginStateData.fromBase64Encoding (encodedData);

            if (pluginStateData.getSize() > 0)
                slot->pluginProcessor->setStateInformation (pluginStateData.getData(), (int)pluginStateData.getSize());
        }
    }

    // Restore selection
    if (selectedRack >= 0 && selectedRack < rackList.getNumRacks())
        rackList.selectRack (selectedRack);

    // Sync to processor
    syncFxChainToProcessor();
}
