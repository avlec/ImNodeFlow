#pragma once

#include "ImNodeFlow.h"

namespace ImFlow
{
    inline void smart_bezier(const ImVec2& p1, const ImVec2& p2, ImU32 color, float thickness)
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x) delta += 0.2f * (p1.x - p2.x);
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        if (p2.x < p1.x - 50.f) delta *= -1.f;
        ImVec2 p11 = p1 + ImVec2(delta, vert);
        dl->AddBezierCubic(p1, p11, p22, p2, color, thickness);
    }

    inline bool smart_bezier_collider(const ImVec2& p, const ImVec2& p1, const ImVec2& p2, float radius)
    {
        float distance = sqrt(pow((p2.x - p1.x), 2.f) + pow((p2.y - p1.y), 2.f));
        float delta = distance * 0.45f;
        if (p2.x < p1.x) delta += 0.2f * (p1.x - p2.x);
        // float vert = (p2.x < p1.x - 20.f) ? 0.062f * distance * (p2.y - p1.y) * 0.005f : 0.f;
        float vert = 0.f;
        ImVec2 p22 = p2 - ImVec2(delta, vert);
        if (p2.x < p1.x - 50.f) delta *= -1.f;
        ImVec2 p11 = p1 + ImVec2(delta, vert);
        return ImProjectOnCubicBezier(p, p1, p11, p22, p2).Distance < radius;
    }

    // -----------------------------------------------------------------------------------------------------------------
    // HANDLER

    template<typename T, typename... Params>
    T* ImNodeFlow::addNode(const std::string& name, const ImVec2& pos, Params&&... args)
    {
        static_assert(std::is_base_of<BaseNode, T>::value, "Pushed type is not a subclass of BaseNode!");
        m_nodes.emplace_back(std::make_shared<T>(name, pos, this, std::forward<Params>(args)...));
        return static_cast<T*>(m_nodes.back().get());
    }

    template<typename T, typename... Params>
    T* ImNodeFlow::placeNode(const std::string& name, Params&&... args)
    {
        return placeNodeAt<T>(name, ImGui::GetMousePos(), std::forward<Params>(args)...);
    }

    template<typename T, typename... Params>
    T* ImNodeFlow::placeNodeAt(const std::string& name, const ImVec2& pos, Params&&... args)
    {
        return addNode<T>(name, screen2content(pos), std::forward<Params>(args)...);
    }

    template <typename T>
    void ImNodeFlow::removeNode(T *const pNode)
    {
        static_assert(std::is_base_of<BaseNode, T>::value, "Popped type is not a subclass of BaseNode!");
        const auto it = std::find_if(std::begin(m_nodes), std::end(m_nodes), [pNode](const std::shared_ptr<BaseNode>& shptr){
            return shptr.get() == pNode;
        });
        if (it != std::end(m_nodes))
            m_nodes.erase(it);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // BASE NODE

    template<typename T>
    InPin<T>* BaseNode::addIN(const std::string& name, T defReturn, ConnectionFilter filter)
    {
        return addIN_uid(name, name, defReturn, filter);
    }

    template<typename T, typename U>
    InPin<T>* BaseNode::addIN_uid(U uid, const std::string& name, T defReturn, ConnectionFilter filter)
    {
        PinUID h = std::hash<U>{}(uid);
        m_ins.emplace_back(std::make_shared<InPin<T>>(h, name, filter, this, defReturn, m_inf));
        return static_cast<InPin<T>*>(m_ins.back().get());
    }

    template<typename T>
    const T& BaseNode::showIN(const std::string& name, T defReturn, ConnectionFilter filter)
    {
        return showIN_uid(name, name, defReturn, filter);
    }

    template<typename T, typename U>
    const T& BaseNode::showIN_uid(U uid, const std::string& name, T defReturn, ConnectionFilter filter)
    {
        PinUID h = std::hash<U>{}(uid);
        for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicIns)
        {
            if (p.second->uid() == h)
            {
                p.first = 1;
                return static_cast<InPin<T>*>(p.second.get())->val();
            }
        }

        m_dynamicIns.emplace_back(std::make_pair(1, std::make_shared<InPin<T>>(h, name, filter, this, defReturn, m_inf)));
        return static_cast<InPin<T>*>(m_dynamicIns.back().second.get())->val();
    }

    template<typename T>
    OutPin<T>* BaseNode::addOUT(const std::string& name, ConnectionFilter filter)
    {
        return addOUT_uid<T>(name, name, filter);
    }

    template<typename T, typename U>
    OutPin<T>* BaseNode::addOUT_uid(U uid, const std::string& name, ConnectionFilter filter)
    {
        PinUID h = std::hash<U>{}(uid);
        m_outs.emplace_back(std::make_shared<OutPin<T>>(h, name, filter, this, m_inf));
        return static_cast<OutPin<T>*>(m_outs.back().get());
    }

    template<typename T>
    void BaseNode::showOUT(const std::string& name, std::function<T()> behaviour, ConnectionFilter filter)
    {
        showOUT_uid<T>(name, name, std::move(behaviour), filter);
    }

    template<typename T, typename U>
    void BaseNode::showOUT_uid(U uid, const std::string& name, std::function<T()> behaviour, ConnectionFilter filter)
    {
        PinUID h = std::hash<U>{}(uid);
        for (std::pair<int, std::shared_ptr<Pin>>& p : m_dynamicOuts)
        {
            if (p.second->uid() == h)
            {
                p.first = 2;
                return;
            }
        }

        m_dynamicOuts.emplace_back(std::make_pair(2, std::make_shared<OutPin<T>>(h, name, filter, this, m_inf)));
        static_cast<OutPin<T>*>(m_dynamicOuts.back().second.get())->behaviour(std::move(behaviour));
    }

    template<typename T, typename U>
    const T& BaseNode::getInVal(U uid)
    {
        PinUID h = std::hash<U>{}(uid);
        auto it = std::find_if(m_ins.begin(), m_ins.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->uid() == h; });
        assert(it != m_ins.end() && "Pin UID not found!");
        return static_cast<InPin<T>*>(it->get())->val();
    }

    template<typename T>
    const T& BaseNode::getInVal(const char* uid)
    {
        PinUID h = std::hash<std::string>{}(std::string(uid));
        auto it = std::find_if(m_ins.begin(), m_ins.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->uid() == h; });
        assert(it != m_ins.end() && "Pin UID not found!");
        return static_cast<InPin<T>*>(it->get())->val();
    }

    template<typename U>
    Pin* BaseNode::inPin(U uid)
    {
        PinUID h = std::hash<U>{}(uid);
        auto it = std::find_if(m_ins.begin(), m_ins.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->uid() == h; });
        assert(it != m_ins.end() && "Pin UID not found!");
        return it->get();
    }

    inline Pin* BaseNode::inPin(const char* uid)
    {
        PinUID h = std::hash<std::string>{}(std::string(uid));
        auto it = std::find_if(m_ins.begin(), m_ins.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->uid() == h; });
        assert(it != m_ins.end() && "Pin UID not found!");
        return it->get();
    }

    template<typename U>
    Pin* BaseNode::outPin(U uid)
    {
        PinUID h = std::hash<U>{}(uid);
        auto it = std::find_if(m_outs.begin(), m_outs.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->uid() == h; });
        assert(it != m_outs.end() && "Pin UID not found!");
        return it->get();
    }

    inline Pin* BaseNode::outPin(const char* uid)
    {
        PinUID h = std::hash<std::string>{}(std::string(uid));
        auto it = std::find_if(m_outs.begin(), m_outs.end(), [&h](std::shared_ptr<Pin>& p)
                            { return p->uid() == h; });
        assert(it != m_outs.end() && "Pin UID not found!");
        return it->get();
    }

    // -----------------------------------------------------------------------------------------------------------------
    // IN PIN

    template<class T>
    const T& InPin<T>::val()
    {
        if(!m_link)
            return m_emptyVal;

        return reinterpret_cast<OutPin<T>*>(m_link->left())->val();
    }

    template<class T>
    void InPin<T>::update()
    {
        // Custom rendering
        if (m_renderer)
        {
            ImGui::BeginGroup();
            m_renderer(this);
            ImGui::EndGroup();
            m_size = ImGui::GetItemRectSize();
            if (ImGui::IsItemHovered())
                m_inf->hovering(this);
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 tl = pinPoint() - ImVec2(m_inf->style().pin_point_empty_hovered_radius, m_inf->style().pin_point_empty_hovered_radius);
        ImVec2 br = pinPoint() + ImVec2(m_inf->style().pin_point_empty_hovered_radius, m_inf->style().pin_point_empty_hovered_radius);

        ImGui::Text(m_name.c_str());
        m_size = ImGui::GetItemRectSize();
        if (ImGui::IsItemHovered())
            draw_list->AddRectFilled(m_pos - m_inf->style().pin_padding, m_pos + m_size + m_inf->style().pin_padding, m_inf->style().colors.pin_hovered, m_inf->style().pin_radius);
        else
            draw_list->AddRectFilled(m_pos - m_inf->style().pin_padding, m_pos + m_size + m_inf->style().pin_padding, m_inf->style().colors.pin_bg, m_inf->style().pin_radius);
        draw_list->AddRect(m_pos - m_inf->style().pin_padding, m_pos + m_size + m_inf->style().pin_padding, m_inf->style().colors.pin_border, m_inf->style().pin_radius, 0, m_inf->style().pin_border_thickness);
        if (m_link)
            draw_list->AddCircleFilled(pinPoint(), m_inf->style().pin_point_radius, m_inf->style().colors.pin_point);
        else
        {
            if (ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(tl, br))
                draw_list->AddCircle(pinPoint(), m_inf->style().pin_point_empty_hovered_radius, m_inf->style().colors.pin_point);
            else
                draw_list->AddCircle(pinPoint(), m_inf->style().pin_point_empty_radius, m_inf->style().colors.pin_point);
        }

        if (ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(tl, br))
            m_inf->hovering(this);
    }

    template<class T>
    void InPin<T>::createLink(Pin *other)
    {
        if (other == this || other->type() == PinType_Input || (m_parent == other->parent() && (m_filter & ConnectionFilter_SameNode) == 0))
            return;

        if (!((m_filter & other->filter()) != 0 || m_filter == ConnectionFilter_None || other->filter() == ConnectionFilter_None)) // Check Filter
            return;

        if (m_link && m_link->left() == other)
        {
            m_link.reset();
            return;
        }

        m_link = std::make_shared<Link>(other, this, m_inf);
        other->setLink(m_link);
        m_inf->addLink(m_link);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // OUT PIN

    template<class T>
    const T &OutPin<T>::val() { return m_val; }

    template<class T>
    void OutPin<T>::update()
    {
        // Custom rendering
        if (m_renderer)
        {
            ImGui::BeginGroup();
            m_renderer(this);
            ImGui::EndGroup();
            m_size = ImGui::GetItemRectSize();
            if (ImGui::IsItemHovered())
                m_inf->hovering(this);
            return;
        }

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 tl = pinPoint() - ImVec2(m_inf->style().pin_point_empty_hovered_radius, m_inf->style().pin_point_empty_hovered_radius);
        ImVec2 br = pinPoint() + ImVec2(m_inf->style().pin_point_empty_hovered_radius, m_inf->style().pin_point_empty_hovered_radius);

        ImGui::SetCursorScreenPos(m_pos);
        ImGui::Text(m_name.c_str());

        m_size = ImGui::GetItemRectSize();
        if (ImGui::IsItemHovered())
            draw_list->AddRectFilled(m_pos - m_inf->style().pin_padding, m_pos + m_size + m_inf->style().pin_padding, m_inf->style().colors.pin_hovered, m_inf->style().pin_radius);
        else
            draw_list->AddRectFilled(m_pos - m_inf->style().pin_padding, m_pos + m_size + m_inf->style().pin_padding, m_inf->style().colors.pin_bg, m_inf->style().pin_radius);
        draw_list->AddRect(m_pos - m_inf->style().pin_padding, m_pos + m_size + m_inf->style().pin_padding, m_inf->style().colors.pin_border, m_inf->style().pin_radius, 0, m_inf->style().pin_border_thickness);
        if (m_links.empty())
        {
            if (ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(tl, br))
                draw_list->AddCircle(pinPoint(), m_inf->style().pin_point_empty_hovered_radius, m_inf->style().colors.pin_point);
            else
                draw_list->AddCircle(pinPoint(), m_inf->style().pin_point_empty_radius, m_inf->style().colors.pin_point);
        }
        else
            draw_list->AddCircleFilled(pinPoint(), m_inf->style().pin_point_radius, m_inf->style().colors.pin_point);

        if (ImGui::IsItemHovered() || ImGui::IsMouseHoveringRect(tl, br))
            m_inf->hovering(this);
    }

    template<class T>
    void OutPin<T>::createLink(ImFlow::Pin *other)
    {
        if (other == this || other->type() == PinType_Output)
            return;

        other->createLink(this);
    }

    template<class T>
    void OutPin<T>::setLink(std::shared_ptr<Link>& link)
    {
        m_links.emplace_back(link);
    }

    template<class T>
    void OutPin<T>::deleteLink()
    {
        m_links.erase(std::remove_if(m_links.begin(), m_links.end(),
                                     [](const std::weak_ptr<Link>& l) { return l.expired(); }), m_links.end());
    }
}
