#include <lvgl.h>
#include <LinkedList.h>
#include "view_i.h"

class ScrollableMenu : public ViewInterface {
public:
    ScrollableMenu(const char* InID)
    {
        ViewID = InID;
    }
    
    ScrollableMenu() 
    {
        
    }

    virtual void Render() override;
    virtual void HandleTouch(TS_Point P) override;
    virtual void HandleAnimations(unsigned long Millis, unsigned long LastTick) override;
    void addItem(const char *text);
private:
    lv_obj_t * List;
};