#pragma once

#include "cards_common.hpp"

#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <list>
#include <string>

namespace cards_renderer
{
class Resource
{
public:
    Resource(const std::string &background_img_path, 
                const std::string &cards_deck_img_path, 
                const std::string &cards_back_img_path)
        : background_img_path_(background_img_path)
        , cards_deck_img_path_(cards_deck_img_path)
        , cards_back_img_path_(cards_back_img_path)
    {
    }
    ~Resource(){}

    const cv::Mat &get_background_image()
    {
        if (background_img_path_.empty())
            return background_image_;
        if (background_image_.empty())
            background_image_ = cv::imread(cards_back_img_path_, cv::IMREAD_COLOR);
        return background_image_;
    }
    cv::Mat get_card_image(const cards_common::Card &card)
    {
        if (cards_deck_image_.empty())
            cards_deck_image_ = cv::imread(cards_deck_img_path_, cv::IMREAD_COLOR);

        const cv::Size card_sz(cards_deck_image_.cols / 13, cards_deck_image_.rows / 4);

        const cv::Point tl(
            ((int)card.value_ - (int)cards_common::CardsValue::ValueNone - 1) * card_sz.width,
            ((int)card.suit_ - (int)cards_common::CardsSuit::SuitNone - 1) * card_sz.height);

        return cards_deck_image_(cv::Rect(tl, card_sz));
    }
    cv::Size get_card_img_size()
    {
        if (cards_deck_image_.empty())
            cards_deck_image_ = cv::imread(cards_deck_img_path_, cv::IMREAD_COLOR);

        return cv::Size(cards_deck_image_.cols / 13, cards_deck_image_.rows / 4);
    }
    const cv::Mat &get_cards_back_image()
    {
        if (cards_back_image_.empty())
            cards_back_image_ = cv::imread(cards_back_img_path_, cv::IMREAD_COLOR);
        return cards_back_image_;
    }
private:
    std::string background_img_path_;
    std::string cards_deck_img_path_;
    std::string cards_back_img_path_;

    /*
        Картинка содержит все карты колоды, размер карты WC x HC. 
        Соответственно размер картинки с колодой: (13 * WC) x (4 * HC)
    */
    cv::Mat cards_deck_image_;
    cv::Mat cards_back_image_;
    cv::Mat background_image_;
};

enum RotateCode
{
    rcNone,
    rc90cw,
    rc180,
    rc90ccw
};

class TableRenderer
{
    struct RenderCard
    {
        cv::Rect rect_ = {};
        RotateCode rotate_ = rcNone;
        cards_common::Card card_ = {};
    };
public:
    TableRenderer(const std::string &background_img_path,
                    const std::string &cards_deck_img_path,
                    const std::string &cards_back_img_path,
                    const cv::Size &table_size)
        : resource_(background_img_path, cards_deck_img_path, cards_back_img_path)
        , table_size_(table_size)
    {
    }
    virtual ~TableRenderer()
    {
    }
public:
    /*
        1. Берем картинку для card
        2. Поворачиваем в зависимости от rotate
        3. Отрисовываем в с левым верхним углом в точке pt
    */
    void add_card(const cv::Point &pt, const cards_common::Card &card, RotateCode rotate = rcNone)
    {
        RenderCard rcard;
        rcard.card_ = card;
        rcard.rotate_ = rotate;
        rcard.rect_ = cv::Rect(pt, resource_.get_card_img_size());
        if (rc90cw == rotate || rc90ccw == rotate)
        {
            int temp = rcard.rect_.width;
            rcard.rect_.width = rcard.rect_.height;
            rcard.rect_.height = temp;
        }
        render_cards_.push_back(rcard);
    }
    /*
        1. Берем картинку для card
        2. Поворачиваем в зависимости от rotate
        3. Отрисовываем в прямоугольник rc (при необходимости масштабируем)
    */
    void add_card(const cv::Rect &rc, const cards_common::Card &card, RotateCode rotate = rcNone)
    {
        RenderCard rcard;
        rcard.card_ = card;
        rcard.rotate_ = rotate;
        rcard.rect_ = rc;
        render_cards_.push_back(rcard);
    }

    const cv::Mat &render()
    {
        render_background();
        for (auto item : render_cards_)
        {
            render_card(item);
        }
        //TODO text or something else
        return table_img_;
    }
public:
    void set_resource(const Resource &resource)
    {
        resource_ = resource;
    }

    void set_table_size(const cv::Size &sz)
    {
        table_size_ = sz;
    }
    const cv::Size &get_table_size() const
    {
        return table_size_;
    }

    cv::Size get_card_size()
    {
        return resource_.get_card_img_size();
    }

    void reset_cards()
    {
        render_cards_.clear();
    }
private:
    Resource resource_;

    cv::Size table_size_;
    cv::Mat table_img_;

    std::list<RenderCard> render_cards_;

    void render_background()
    {
        const cv::Mat &background = resource_.get_background_image();
        if (background.empty())
        {
            table_img_.create(table_size_, CV_8UC3);
            table_img_ = cv::Scalar(0, 128, 0);
            return;
        }
            
        table_img_ = cv::repeat(background, 
                                (int)(table_size_.height / background.rows) + 1,
                                (int)(table_size_.width / background.cols) + 1).colRange(0, table_size_.width).rowRange(0, table_size_.height);
    }
    void render_card(const RenderCard &card)
    {
        const cv::Mat &card_img 
            = (cards_common::CardsSuit::SuitNone == card.card_.suit_ || cards_common::CardsValue::ValueNone == card.card_.value_)
            ? resource_.get_cards_back_image()
            : resource_.get_card_image(card.card_);
            
        const cv::Mat *ptr_img = &card_img;
        cv::Mat rotated;
        switch (card.rotate_)
        {
        case rcNone:
            break;
        case rc90cw:
            cv::rotate(*ptr_img, rotated, cv::ROTATE_90_CLOCKWISE);
            ptr_img = &rotated;
            break;
        case rc180:
            cv::rotate(*ptr_img, rotated, cv::ROTATE_180);
            ptr_img = &rotated;
            break;
        case rc90ccw:
            cv::rotate(*ptr_img, rotated, cv::ROTATE_90_COUNTERCLOCKWISE);
            ptr_img = &rotated;
            break;
        }
        cv::Mat sized;
        if (card.rect_.size() != ptr_img->size())
        {
            cv::resize(*ptr_img, sized, card.rect_.size());
            ptr_img = &sized;
        }

        cv::Rect crop_rc = card.rect_;
        if (crop_rc.x < 0)
        {
            crop_rc.width += crop_rc.x;
            crop_rc.x = -crop_rc.x;
        }
        else
            crop_rc.x = 0;
        if (crop_rc.y < 0)
        {
            crop_rc.height += crop_rc.y;
            crop_rc.y = -crop_rc.y;
        }
        else
            crop_rc.y = 0;

        cv::Rect rc;
        rc.x = std::max(card.rect_.x, 0);
        rc.y = std::max(card.rect_.y, 0);

        rc.width = crop_rc.width = std::min(rc.x + crop_rc.width, table_size_.width) - rc.x;
        rc.height = crop_rc.height = std::min(rc.y + crop_rc.height, table_size_.height) - rc.y;
        if (crop_rc.width <= 0 || crop_rc.height <= 0)
            return;
        cv::Mat croped = (*ptr_img)(crop_rc);
        croped.copyTo(table_img_(rc));
    }
};
};