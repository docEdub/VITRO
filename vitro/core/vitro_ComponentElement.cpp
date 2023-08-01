namespace vitro {

JSClassID ComponentElement::jsClassID = 0;

//==============================================================================

ComponentElement::MouseEventsProxy::MouseEventsProxy(ComponentElement& el)
    : element {el}
{
    if (auto* component{ element.getComponent() })
        component->addMouseListener(this, true);
}

void ComponentElement::MouseEventsProxy::mouseMove(const MouseEvent& event)
{
    element.handleMouseMove(event);
}

void ComponentElement::MouseEventsProxy::mouseEnter(const MouseEvent& event)
{
    element.setAttribute(attr::hover, true);
    element.handleMouseEnter(event);
}

void ComponentElement::MouseEventsProxy::mouseExit(const MouseEvent& event)
{
    element.setAttribute(attr::hover, false);
    element.handleMouseExit(event);
}

void ComponentElement::MouseEventsProxy::mouseDown(const MouseEvent& event)
{
    element.setAttribute(attr::active, true);
    element.handleMouseDown(event);
}

void ComponentElement::MouseEventsProxy::mouseUp(const MouseEvent& event)
{
    element.setAttribute(attr::active, false);
    element.handleMouseUp(event);
}

//==============================================================================

ComponentElement::ComponentElement(const Identifier& tag, Context& ctx)
    : LayoutElement(tag, ctx)
{
    registerStyleProperty(attr::css::alpha);
    registerStyleProperty(attr::css::click_through);
    registerStyleProperty(attr::css::cursor);
    registerStyleProperty(attr::css::shadow_color);
    registerStyleProperty(attr::css::shadow_radius);
    registerStyleProperty(attr::css::shadow_offset_x);
    registerStyleProperty(attr::css::shadow_offset_y);
}

ComponentElement::Ptr ComponentElement::getParentComponentElement()
{
    Element::Ptr p{ parent.lock() };

    while (p != nullptr) {
        if (p->isComponentElement())
            return std::dynamic_pointer_cast<ComponentElement>(p);

        p = p->getParentElement();
    }

    return nullptr;
}

void ComponentElement::updateComponentBoundsToLayoutNode()
{
    if (auto* component{ getComponent() }) {
        auto targetBounds{ getLayoutElementBounds().toNearestInt() };

        component->setBounds(targetBounds);
    }
}

void ComponentElement::registerJSPrototype(JSContext* ctx, JSValue prototype)
{
    LayoutElement::registerJSPrototype(ctx, prototype);

    registerJSProperty(ctx, prototype, "viewBounds", &js_getViewBounds);
    registerJSProperty(ctx, prototype, "screenBounds", &js_getScreenBounds);
}

void ComponentElement::setColourFromStyleProperty(juce::Component& component, int colourId, const Identifier& propertyName)
{
    if (const auto&& [changed, colour]{ getStylePropertyChanged(propertyName) }; changed && !colour.isVoid())
        component.setColour(colourId, parseColourFromString(colour.toString()));
    else if (colour.isVoid())
        component.removeColour(colourId);
}

void ComponentElement::setColourFromStyleProperty(int colourId, const Identifier& propertyName)
{
    if (auto* component{ getComponent() })
        setColourFromStyleProperty(*component, colourId, propertyName);
}

void ComponentElement::populateFontFromStyleProperties(juce::Font& font)
{
    if (const auto&& [changed, fontFamily]{ getStylePropertyChanged(attr::css::font_family) }; changed && !fontFamily.isVoid())
        font.setTypefaceName(fontFamily.toString());

    if (const auto&& [changed, fontStyle]{ getStylePropertyChanged(attr::css::font_style) }; changed && !fontStyle.isVoid()) {
        StringArray styleAttrs{};
        styleAttrs.addTokens(fontStyle.toString(), false);

        bool bold{ false };
        bool italic{ false };
        bool underline{ false };

        for (const auto& attr : styleAttrs) {
            const auto style{ attr.toLowerCase() };

            if (style == "bold")
                bold = true;
            else if (style == "italic")
                italic = true;
            else if (style == "underline")
                underline = true;
        }

        font.setBold(bold);
        font.setItalic(italic);
        font.setUnderline(underline);

    } else if (changed && fontStyle.isVoid()) {
        // Set default style
        font.setBold(false);
        font.setItalic(false);
        font.setUnderline(false);
    }

    if (const auto&& [changed, fontSize]{ getStylePropertyChanged(attr::css::font_size) }; changed && !fontSize.isVoid())
        font.setHeight(static_cast<float>(fontSize));

    if (const auto&& [changed, fontKerning]{ getStylePropertyChanged(attr::css::font_kerning) }; changed && !fontKerning.isVoid())
        font.setExtraKerningFactor(static_cast<float>(fontKerning));
}

void ComponentElement::setMouseCursorFromStyleProperties()
{
    const static std::map<String, MouseCursor::StandardCursorType> cursorTypes {
        { "none",      MouseCursor::NoCursor           },
        { "auto",      MouseCursor::NormalCursor       },
        { "wait",      MouseCursor::WaitCursor         },
        { "pointer",   MouseCursor::PointingHandCursor },
        { "hand",      MouseCursor::PointingHandCursor },
        { "crisshair", MouseCursor::CrosshairCursor    },
        { "copy",      MouseCursor::CopyingCursor      },
        { "drag",      MouseCursor::DraggingHandCursor }
    };

    if (auto* component{ getComponent() }) {
        if (const auto&& [changed, cursor]{ getStylePropertyChanged(attr::css::cursor) }; changed && ! cursor.isVoid())
        {
            auto cur{ MouseCursor::NormalCursor };
            auto it{ cursorTypes.find(cursor.toString().trim().toLowerCase()) };

            if (it != cursorTypes.end())
                cur = it->second;

            component->setMouseCursor(cur);
        }
    }
}

void ComponentElement::initialize()
{
    LayoutElement::initialize();

    mouseEventsProxy = std::make_unique<MouseEventsProxy>(*this);

    // Install the listener to track component move and resize.
    if (auto* comp{ this->getComponent() })
        comp->addComponentListener(this);
}

void ComponentElement::update()
{
    LayoutElement::update();

    if (auto* comp{ getComponent() }) {
        if (const auto&& [changed, val]{ getAttributeChanged(attr::enabled) }; changed && !val.isVoid())
            comp->setEnabled(val);

        if (const auto&& [changed, val]{ getAttributeChanged(attr::visible) }; changed && !val.isVoid())
            comp->setVisible(val);

        if (const auto&& [changed, val]{getStylePropertyChanged(attr::css::alpha) }; changed)
            comp->setAlpha(val.isVoid() ? 1.0f : float(val));

        if (const auto&& [changed, val]{ getStylePropertyChanged(attr::css::click_through) }; changed)
            comp->setInterceptsMouseClicks(val.isVoid() ? true : bool(val), false);
    }

    // cursor
    setMouseCursorFromStyleProperties();

    bool createShadow{ false };

    // shadow-color
    if (const auto&& [changed, prop]{ getStylePropertyChanged(attr::css::shadow_color) }; changed) {
        if (prop.isVoid()) {
            dropShadower.reset();
        } else {
            createShadow = true;
            shadow.colour = parseColourFromString(prop.toString());
        }
    }

    // shadow-radius
    if (const auto&& [changed, prop]{ getStylePropertyChanged(attr::css::shadow_radius) }; changed)
        shadow.radius = prop.isVoid() ? 4 : int(prop);

    // shadow-offset-x
    if (const auto&& [changed, prop]{ getStylePropertyChanged(attr::css::shadow_offset_x) }; changed)
        shadow.offset.setX(prop.isVoid() ? 0 : int(prop));

    // shadow-offset-y
    if (const auto&& [changed, prop]{ getStylePropertyChanged(attr::css::shadow_offset_y) }; changed)
        shadow.offset.setY(prop.isVoid() ? 0 : int(prop));

    if (createShadow && dropShadower == nullptr) {
        dropShadower = std::make_unique<DropShadower>(shadow);
        dropShadower->setOwner(getComponent());
    }
}

void ComponentElement::reconcileElement()
{
    LayoutElement::reconcileElement();

    juce::Component* thisComponent{ getComponent() };
    jassert(thisComponent != nullptr);

    if (parent.lock() == nullptr) {
        if (auto owner{ thisComponent->getParentComponent() })
            owner->removeChildComponent(thisComponent);
    } else {
        if (thisComponent->getParentComponent() == nullptr) {
            if (auto parentComponentElement{ getParentComponentElement() }) {
                // @note When appending to a scroll area this component must added to
                //       the scrollable container, not to the actual parent component.
                parentComponentElement->getContainerComponent()->addAndMakeVisible(thisComponent);
            }
        }
    }
}

void ComponentElement::handleMouseMove(const MouseEvent&)
{
    evaluateAttributeScript(attr::onmousemove);
}

void ComponentElement::handleMouseEnter(const MouseEvent&)
{
    evaluateAttributeScript(attr::onmouseenter);
};

void ComponentElement::handleMouseExit(const MouseEvent&)
{
    evaluateAttributeScript(attr::onmouseexit);
};

void ComponentElement::handleMouseDown(const MouseEvent&)
{
    evaluateAttributeScript(attr::onmousedown);
};

void ComponentElement::handleMouseUp(const MouseEvent&)
{
    evaluateAttributeScript(attr::onmouseup);
};

void ComponentElement::componentMovedOrResized(Component&, bool wasMoved, bool wasResized)
{
    if (wasMoved)
        evaluateAttributeScript(attr::onmove);

    if (wasResized)
        evaluateAttributeScript(attr::onresize);
}

//==============================================================================

// Convert local bounds to the top view realtive
JSValue ComponentElement::js_getViewBounds(JSContext* ctx, JSValueConst self)
{
    juce::DynamicObject::Ptr rectObj{ new DynamicObject() };
    juce::Rectangle<float> bounds{};

    if (auto element{ Context::getJSNativeObject<Element>(self) }) {
        if (auto rootElement{ element->getTopLevelElement() }) {
            if (rootElement->isComponentElement()) {
                if (auto componentRoot{ std::dynamic_pointer_cast<ComponentElement>(rootElement) }) {
                    if (auto* rootComponent{ componentRoot->getComponent() }) {
                        if (auto componentElement{ std::dynamic_pointer_cast<ComponentElement>(element) }) {
                            if (auto* component{ componentElement->getComponent()})
                                bounds = rootComponent->getLocalArea(component, componentElement->getLayoutElementBounds());
                        }
                    }
                }
            }
        }
    }

    rectObj->setProperty(attr::x, bounds.getX());
    rectObj->setProperty(attr::y, bounds.getY());
    rectObj->setProperty(attr::width, bounds.getWidth());
    rectObj->setProperty(attr::height, bounds.getHeight());

    return js::varToJSValue(ctx, juce::var(rectObj));
}

// Convert local bound to screen relative
JSValue ComponentElement::js_getScreenBounds(JSContext* ctx, JSValueConst self)
{
    juce::DynamicObject::Ptr rectObj{ new DynamicObject() };
    juce::Rectangle<float> bounds{};

    if (auto element{ Context::getJSNativeObject<Element>(self) }) {
        if (auto componentElement{ std::dynamic_pointer_cast<ComponentElement>(element) }) {
            if (auto* component{ componentElement->getComponent()})
                bounds = component->localAreaToGlobal(componentElement->getLayoutElementBounds());
        }
    }

    rectObj->setProperty(attr::x, bounds.getX());
    rectObj->setProperty(attr::y, bounds.getY());
    rectObj->setProperty(attr::width, bounds.getWidth());
    rectObj->setProperty(attr::height, bounds.getHeight());

    return js::varToJSValue(ctx, juce::var(rectObj));
}

} // namespace vitro
