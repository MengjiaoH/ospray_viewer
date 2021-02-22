#include "widget.h"

Widget::Widget(float begin, float end, float default_iso, std::vector<Bar> bars)
    :range_start{begin}, range_end(end), iso(default_iso), bars(bars)
{}

void Widget::draw()
{
    ImGui::SliderFloat("Delta", &iso, range_start, range_end); 
    if(range_start != pre_iso){
        isoValueChanged = true;
    }else{
        // std::cout << "current time step " << currentTimeStep << " and pre time step " << preTimeStep << std::endl; 
        isoValueChanged = false;
    }

    if(isoValueChanged && lock.try_lock()){
        // std::cout << "current time step " << currentTimeStep << " and pre time step " << preTimeStep << std::endl; 
        pre_iso = iso;
        lock.unlock();
    }
    // ImGui::Checkbox("Show Iso-surfaces", &show_isosurfaces); 
    ImGui::Checkbox("Show Iso-surfaces", &show_isosurfaces);
    float space = 2;
    float height = 10;
    
    // if (ImGui::TreeNode("Barcodes"))
    // {
    //     // ImGui::BeginChild("##colors", ImVec2(0, 0), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NavFlattened);
    //     // const float footer_height_to_reserve = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
        
    //     // ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar);
    //     ImGui::BeginChild("Scrolling");
    //     for(int i = 0; i < bars.size(); i++){
    //         ImVec2 p = ImGui::GetCursorScreenPos();
    //         // std::cout << "p" << p.x << " " << p.y << std::endl;
    //         float x = p.x ;
    //         float y = p.y + i * (height + space);
    //         float diff = bars[i].birth - bars[i].death;
    //         ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + diff, y + height), IM_COL32(252, 94, 3, 255));
    //         // ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 40, p.y + 40), ImVec2(p.x + 70, p.y + 70), IM_COL32_WHITE);
    //     }
    //     // for (int n = 0; n < 50; n++)
    //     //     ImGui::Text("%04d: Some text", n);
    //     ImGui::EndChild();
    //     ImGui::TreePop();
        
    // }
    if (ImGui::TreeNode("Barcodes"))
    {
        // Vertical scroll functions
        // HelpMarker("Use SetScrollHereY() or SetScrollFromPosY() to scroll to a given vertical position.");

        static int track_item = 50;
        static bool enable_track = true;
        static bool enable_extra_decorations = false;
        static float scroll_to_off_px = 0.0f;
        static float scroll_to_pos_px = 200.0f;

        ImGuiStyle& style = ImGui::GetStyle();
        float child_w = (ImGui::GetContentRegionAvail().x - 4 * style.ItemSpacing.x) / 5;
        if (child_w < 1.0f)
            child_w = 1.0f;
        ImGui::PushID("##VerticalScrolling");
        for(int i = 0; i < bars.size(); i++){
            ImVec2 p = ImGui::GetCursorScreenPos();
            // ImGui::TextUnformatted();
            // std::cout << "p" << p.x << " " << p.y << std::endl;
            ImGui::Text("%.3f, %.3f", bars[i].birth, bars[i].death);
            // ImGui::SameLine(140);
            float x = p.x + 140;
            float y = p.y + i * (height + space);
            float diff = bars[i].birth - bars[i].death;
            ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + diff * 3, y + height), IM_COL32(252, 94, 3, 255));
            // ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(p.x + 40, p.y + 40), ImVec2(p.x + 70, p.y + 70), IM_COL32_WHITE);
        }


            // ImGui::BeginGroup();
            // const char* names[] = { "Top", "25%", "Center", "75%", "Bottom" };
            // ImGui::TextUnformatted(names[0]);

            // const ImGuiWindowFlags child_flags = enable_extra_decorations ? ImGuiWindowFlags_MenuBar : 0;
            // const ImGuiID child_id = ImGui::GetID((void*)(intptr_t)0);
            // const bool child_is_visible = ImGui::BeginChild(child_id, ImVec2(child_w, 200.0f), true, child_flags);
            // if (ImGui::BeginMenuBar())
            // {
            //     ImGui::TextUnformatted("abc");
            //     ImGui::EndMenuBar();
            // }
            // if (scroll_to_off)
            //     ImGui::SetScrollY(scroll_to_off_px);
            // if (scroll_to_pos)
            //     ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + scroll_to_pos_px, 0 * 0.25f);
            // if (child_is_visible) // Avoid calling SetScrollHereY when running with culled items
            // {
            //     for (int item = 0; item < 100; item++)
            //     {
            //         if (enable_track && item == track_item)
            //         {
            //             ImGui::TextColored(ImVec4(1, 1, 0, 1), "Item %d", item);
            //             ImGui::SetScrollHereY(0 * 0.25f); // 0.0f:top, 0.5f:center, 1.0f:bottom
            //         }
            //         else
            //         {
            //             ImGui::Text("Item %d", item);
            //         }
            //     }
            // }
            // float scroll_y = ImGui::GetScrollY();
            // float scroll_max_y = ImGui::GetScrollMaxY();
            // ImGui::EndChild();
            // ImGui::Text("%.0f/%.0f", scroll_y, scroll_max_y);
            // ImGui::EndGroup();
     
        ImGui::PopID();
        ImGui::TreePop();
    }
}

bool Widget::changed(){
    return isoValueChanged;
}

float Widget::getIsoValue(){
    return iso;
}

std::vector<Bar> getBarcode()
{
    std::ifstream jsonFile("/home/mengjiao/Desktop/projects/cosmic-void-viewer/data/real_original.json");
    if(!jsonFile){
        std::cout << "Cannot open json file" << std::endl;
    }
    json parsed_json = nlohmann::json::parse(jsonFile);

    std::vector<Bar> bars;
    for (json::iterator it = parsed_json.begin(); it != parsed_json.end(); ++it) {
        auto value = it.value();
        Bar temp_bar(value["birth"], value["death"]);
        bars.push_back(temp_bar);
    }
    // sort bars 
    std::sort(bars.begin(), bars.end(), [](const Bar &a, const Bar &b){
        float a_diff = a.birth - a.death;
        float b_diff = b.birth - b.death;
        return a_diff > b_diff;
    });

    // for(int i = 0; i < bars.size(); i++){
    //     std::cout << bars[i].birth - bars[i].death << std::endl;
    // }

    return bars;
}

