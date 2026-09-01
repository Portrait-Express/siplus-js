#include "value.hpp"
#include "util.hpp"
#include "type.hpp"



struct JsFunctionValueRetriever : ValueRetriever {
    JsFunctionValueRetriever(
        std::weak_ptr<SIPlusParserContext> context,
        std::shared_ptr<ValueRetriever> parent,
        std::vector<std::shared_ptr<ValueRetriever>> parameters,
        emscripten::val impl
    );

    SIPlus::UnknownDataTypeContainer retrieve(SIPlus::InvocationContext& value) const override;

private:
    std::weak_ptr<SIPlusParserContext> context_;
    std::shared_ptr<ValueRetriever> parent_;
    std::vector<std::shared_ptr<ValueRetriever>> parameters_;
    emscripten::val impl_;
};






JsFunctionImpl::JsFunctionImpl(
    std::weak_ptr<SIPlusParserContext> context,
    emscripten::val impl
) : context_(context), impl_(impl) {
    assert_typeof("function_impl", impl_, "function");
}

std::shared_ptr<ValueRetriever> JsFunctionImpl::value(
    std::shared_ptr<ValueRetriever> parent, 
    std::vector<std::shared_ptr<ValueRetriever>> parameters
) const {
    auto ctx = context_.lock();
    return std::make_shared<JsFunctionValueRetriever>(ctx, parent, parameters, impl_);
}






JsFunctionValueRetriever::JsFunctionValueRetriever(
    std::weak_ptr<SIPlusParserContext> context,
    std::shared_ptr<ValueRetriever> parent,
    std::vector<std::shared_ptr<ValueRetriever>> parameters,
    emscripten::val impl
) : context_(context), parent_(parent), parameters_(parameters), impl_(impl) {
    assert_typeof("function_impl", impl_, "function");
}

SIPlus::UnknownDataTypeContainer JsFunctionValueRetriever::retrieve(
    SIPlus::InvocationContext& value
) const {
    auto ctx = context_.lock();
    auto arr = emscripten::val::global("Array").new_(parameters_.size() + 2);

    //Base value
    arr.set(0, ctx->convert<JSType>(value.default_data()).as<JSType>());

    //Parent value
    auto parentVal = parent_->retrieve(value);
    arr.set(1, ctx->convert<JSType>(parentVal).as<JSType>());

    //Set parameters
    for(int i = 0; i < parameters_.size(); i++) {
        auto paramVal = parameters_[i]->retrieve(value);
        arr.set(i + 2, ctx->convert<JSType>(paramVal).as<JSType>());
    }

    //Invoke function
    auto ret = impl_.call<emscripten::val>("apply", emscripten::val::null(), arr);
    
    return jsToCpp(ret);
}
