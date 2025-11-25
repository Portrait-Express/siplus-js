#include <emscripten/bind.h>
#include <vector>

#include "siplus/context.h"
#include "siplus/stl.h"
#include "siplus/stl/converters/numeric.h"
#include "siplus/text/accessor.h"
#include "siplus/text/converter.h"
#include "siplus/text/data.h"
#include "siplus/text/iterator.h"
#include "siplus/util.h"

#include "stdlib.h"

SIPlus::text::UnknownDataTypeContainer decay(emscripten::val val) {
    if(val.isString()) {
        return SIPlus::text::make_data(val.as<std::string>());
    } else if(val.isNumber()) {
        return SIPlus::text::make_data(val.as<double>());
    } else if(val.isTrue() || val.isFalse()) {
        return SIPlus::text::make_data(val.as<bool>());
    } else if(val.isNull() || val.isUndefined()) {
        return SIPlus::text::UnknownDataTypeContainer{};
    } else {
        return SIPlus::text::make_data(val);
    }
}

struct JsAccessor : SIPlus::text::Accessor {
    SIPlus::text::UnknownDataTypeContainer 
    access(const SIPlus::text::UnknownDataTypeContainer &value, 
           const std::string &name) override {
        emscripten::val result;
        const emscripten::val& jVal = value.as<emscripten::val>();

        auto type = jVal.typeOf().as<std::string>();
        if(type == "object") {
            result = jVal[name];
        } else {
            throw std::runtime_error{
                to_string("Cannot access property ", name, 
                          " off object of type ", get_type_name(value.type),
                          "/", type)};
        }

        return decay(result);
    }

    bool can_access(const SIPlus::text::UnknownDataTypeContainer &value) override {
        return value.is<emscripten::val>();
    }
};

struct JsStringConverter : SIPlus::text::Converter {
    SIPlus::text::UnknownDataTypeContainer 
    convert(const SIPlus::text::UnknownDataTypeContainer& from, std::type_index to) const override {
        auto& val = from.as<emscripten::val>();
        auto string = val.call<emscripten::val>("toString");
        return SIPlus::text::make_data(string.as<std::string>());
    }

    bool can_convert(std::type_index from, std::type_index to) const override {
        return from == typeid(emscripten::val) && to == typeid(std::string);
    }
};

struct JsIterator : SIPlus::text::IteratorProvider {
    bool can_iterate(const SIPlus::text::UnknownDataTypeContainer &value) override {
        return value.is<emscripten::val>() && value.as<emscripten::val>().isArray();
    }

    std::unique_ptr<SIPlus::text::Iterator> 
    iterator(const SIPlus::text::UnknownDataTypeContainer &value) override {
        impl *iterator = new impl{value.as<emscripten::val>()};
        return std::unique_ptr<impl>{iterator};
    }

    struct impl : SIPlus::text::Iterator {
        impl(emscripten::val value) : value_(value)
            , length_(value["length"].as<int>()) { }

        void next() override {
            i_++;
        }

        bool more() override {
            return i_ < length_ - 1;
        }

        SIPlus::text::UnknownDataTypeContainer current() override {
            return SIPlus::text::make_data(value_[i_]);
        }

    private:
        int i_ = -1;
        int length_;
        emscripten::val value_;
    };
};

struct JsArrayConverter : SIPlus::text::Converter {
    JsArrayConverter(std::shared_ptr<SIPlus::SIPlusParserContext> context) 
        : context_(context) {}

    bool can_convert(std::type_index from, std::type_index to) const override {
        return from == typeid(std::vector<SIPlus::text::UnknownDataTypeContainer>)
            && to == typeid(emscripten::val);
    }

    SIPlus::text::UnknownDataTypeContainer 
    convert(const SIPlus::text::UnknownDataTypeContainer& from, std::type_index to) const override {
        emscripten::val arr = emscripten::val::global("Array").new_();

        for(auto value : from.as<std::vector<SIPlus::text::UnknownDataTypeContainer>>()) {
            if(!value.is<emscripten::val>()) {
                value = context_->convert<emscripten::val>(value);
            }

            arr.call<void>("push", value.as<emscripten::val>());
        }

        return SIPlus::text::make_data(arr);
    }

private:
    std::shared_ptr<SIPlus::SIPlusParserContext> context_;
};

struct JsPrimitiveConverter : SIPlus::text::Converter {
    SIPlus::text::UnknownDataTypeContainer 
    convert(const SIPlus::text::UnknownDataTypeContainer& from, std::type_index to) const override {
        if(int_converter_.can_convert(from.type, typeid(long))) {
            long numVal = int_converter_.convert(from, typeid(long)).as<long>();
            return SIPlus::text::make_data(emscripten::val{numVal});
        } else if(float_converter_.can_convert(from.type, typeid(double))) {
            double floatVal = float_converter_.convert(from, typeid(double)).as<double>();
            return SIPlus::text::make_data(emscripten::val{floatVal});
        } else if(from.is<std::string>()) {
            return SIPlus::text::make_data(emscripten::val{from.as<std::string>()});
        } else {
            throw std::runtime_error{"Cannot convert from " + get_type_name(from.type) + " to emscripten::val"};
        }
    }
    
    bool can_convert(std::type_index from, std::type_index to) const override {
        return to == typeid(emscripten::val) && (
            int_converter_.can_convert(from, typeid(long)) ||
            float_converter_.can_convert(from, typeid(double)) ||
            from == typeid(std::string)
        );
    }

private:
    SIPlus::stl::int_converter int_converter_;
    SIPlus::stl::float_converter float_converter_;
};

int attach_stl(std::shared_ptr<SIPlus::SIPlusParserContext> context) {
    context->emplace_accessor<JsAccessor>();

    context->emplace_converter<JsStringConverter>();
    context->emplace_converter<JsArrayConverter>(context);
    context->emplace_converter<JsPrimitiveConverter>();

    context->emplace_iterator<JsIterator>();

    SIPlus::stl::attach_stl(*context);

    return 0;
}
