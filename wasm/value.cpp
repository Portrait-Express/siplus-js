#include "value.hpp"
#include "util.hpp"
#include "type.hpp"



struct JsFunctionValueRetriever : ValueRetriever {
    JsFunctionValueRetriever(
        std::shared_ptr<ValueRetriever> parent,
        std::vector<std::shared_ptr<ValueRetriever>> parameters,
        emscripten::val impl
    );

    SIPlus::UnknownDataTypeContainer retrieve(SIPlus::InvocationContext& value) const override;

private:
    std::shared_ptr<ValueRetriever> parent_;
    std::vector<std::shared_ptr<ValueRetriever>> parameters_;
    emscripten::val impl_;
};






JsFunctionImpl::JsFunctionImpl(emscripten::val impl) : impl_(impl) {
    assert_typeof("function_impl", impl_, "function");
}

std::shared_ptr<ValueRetriever> JsFunctionImpl::value(
    std::shared_ptr<ValueRetriever> parent, 
    std::vector<std::shared_ptr<ValueRetriever>> parameters
) const {
    return std::make_shared<JsFunctionValueRetriever>(parent, parameters, impl_);
}






JsFunctionValueRetriever::JsFunctionValueRetriever(
    std::shared_ptr<ValueRetriever> parent,
    std::vector<std::shared_ptr<ValueRetriever>> parameters,
    emscripten::val impl
) : parent_(parent), parameters_(parameters), impl_(impl) {
    assert_typeof("function_impl", impl_, "function");
}

SIPlus::UnknownDataTypeContainer JsFunctionValueRetriever::retrieve(
    SIPlus::InvocationContext& value
) const {
    auto arr = emscripten::val::global("Array").new_(parameters_.size() + 2);

    //Base value
    arr.set(0, cppToJs(value.default_data()));

    //Parent value
    auto parentVal = parent_->retrieve(value);
    arr.set(1, cppToJs(parentVal));

    //Set parameters
    for(int i = 0; i < parameters_.size(); i++) {
        auto paramVal = parameters_[i]->retrieve(value);
        arr.set(i + 2, cppToJs(paramVal));
    }

    //Invoke function
    auto ret = impl_.call<emscripten::val>("apply", emscripten::val::null(), arr);
    
    return jsToCpp(ret);
}
