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

    template<typename T>
    void ImNodeFlow::addNode(const std::string& name, const ImVec2&& pos)
    {
        static_assert(std::is_base_of<BaseNode, T>::value, "Pushed type is not subclass of BaseNode!");
        m_nodes.emplace_back(std::make_shared<T>(name, pos, this));
    }

    // -----------------------------------------------------------------------------------------------------------------
    // BASE NODE

    template<typename T>
    void BaseNode::addIN(const std::string name, T defReturn, ConnectionFilter filter)
    {
        m_ins.emplace_back(std::make_shared<InPin<T>>(name, filter, PinKind_Input, this, defReturn, m_inf));
    }

    template<typename T>
    void BaseNode::addOUT(const std::string name, ConnectionFilter filter)
    {
        m_outs.emplace_back(std::make_shared<OutPin<T>>(name, filter, PinKind_Output, this, m_inf));
    }

    template<typename T>
    InPin<T>& BaseNode::ins(int i) { return *std::dynamic_pointer_cast<InPin<T>>(m_ins[i]); }
    template<typename T>
    OutPin<T>& BaseNode::outs(int i) { return *std::dynamic_pointer_cast<OutPin<T>>(m_outs[i]); }

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
        draw();

        if (ImGui::IsItemHovered())
            m_inf->hovering(this);
    }

    template<class T>
    void InPin<T>::draw()
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImGui::Text(m_name.c_str());
        m_size = ImGui::GetItemRectSize();
        if (ImGui::IsItemHovered())
            draw_list->AddRectFilled(m_pos - ImVec2(3,1), m_pos + m_size + ImVec2(3,2), IM_COL32(100, 100, 255, 70));
        draw_list->AddRect(m_pos - ImVec2(3,1), m_pos + m_size + ImVec2(3,2), IM_COL32(255, 255, 255, 100));
    }

    template<class T>
    void InPin<T>::createLink(Pin *left)
    {
        m_link = std::make_shared<Link>(left, this, m_inf);
        left->setLink(m_link);
    }

    // -----------------------------------------------------------------------------------------------------------------
    // OUT PIN

    template<class T>
    const T &OutPin<T>::val() { m_val = m_behaviour(); return m_val; } // TODO: Resolve ME somewhere else so it's not done every frame

    template<class T>
    void OutPin<T>::update()
    {
        draw();

        if (ImGui::IsItemHovered())
            m_inf->hovering(this);
    }

    template<class T>
    void OutPin<T>::draw()
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImGui::Text(m_name.c_str());
        m_size = ImGui::GetItemRectSize();
        if (ImGui::IsItemHovered())
            draw_list->AddRectFilled(m_pos - ImVec2(3,1), m_pos + m_size + ImVec2(3,2), IM_COL32(100, 100, 255, 70));
        draw_list->AddRect(m_pos - ImVec2(3,1), m_pos + m_size + ImVec2(3,2), IM_COL32(255, 255, 255, 100));
    }
}
