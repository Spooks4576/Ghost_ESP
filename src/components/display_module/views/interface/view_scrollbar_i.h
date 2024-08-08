#include <lvgl.h>
#include <LinkedList.h>
#include "view_i.h"


LV_IMG_DECLARE(Arrow);

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
    lv_obj_t * BackBtn;
    lv_obj_t * ConfirmButton;
    lv_obj_t * MoveSelectUpBtn;
    lv_obj_t* MoveSelectDownBtn;
};