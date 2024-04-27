#include <lvgl.h>
#include <LinkedList.h>
#include "view_i.h"

struct SubMenuItem
{
    lv_obj_t * cont;
    lv_obj_t * label;
    lv_obj_t * GradientLayer;
};

class ScrollableMenu : public ViewInterface {
public:

    virtual void Render() override;
    virtual void HandleTouch(TS_Point P) override;
    virtual void HandleAnimations(unsigned long Millis, unsigned long LastTick) override;
    void addItem(const char *text);
    void selectItemCallback(lv_obj_t *obj, lv_event_t event);
private:
    lv_obj_t * List;
    LinkedList<SubMenuItem> SubMenuItems;
};