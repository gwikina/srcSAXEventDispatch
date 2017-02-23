#include <srcSAXEventDispatchUtilities.hpp>

#include <TypePolicySingleEvent.hpp>
#include <NamePolicySingleEvent.hpp>
#include <ParamTypePolicySingleEvent.hpp>

#include <string>
#include <vector>

#ifndef INCLUDED_FUNCTION_POLICY_SINGE_EVENT_HPP
#define INCLUDED_FUNCTION_POLICY_SINGE_EVENT_HPP

class FunctionPolicy : public srcSAXEventDispatch::EventListener, public srcSAXEventDispatch::PolicyDispatcher, public srcSAXEventDispatch::PolicyListener {

public:

    enum FunctionType { CONSTRUCTOR, DESTURCTOR, OPERATOR, FUNCTION };

    struct FunctionData {

        FunctionType type;
        std::string stereotype;

        TypePolicy::TypeData * returnType;
        NamePolicy::NameData * name;

        std::vector<ParamTypePolicy::ParamTypeData *> parameters;
        std::vector<DeclTypePolicy::DeclTypeData *> relations;

        bool isVirtual;
        bool isPureVirtual;
        bool isConst;
        bool isStatic;
        bool isInline;
        bool isFinal;
        bool isOverride;
        bool isConstExpr;
        bool isDelete;

        std::string ToString() const {

            std::string signature = name->ToString();
            signature += '(';
            for(std::size_t pos = 0; pos < parameters.size(); ++pos) {
                if(pos > 0)
                    signature += ", ";
                signature += parameters[pos]->type->ToString();
            }
            signature += ')';
            if(isConst)
                signature += " const";

            return signature;

        }

        friend std::ostream & operator<<(std::ostream & out, const FunctionData & functionData) {

            out << *functionData.returnType << ' ' << *functionData.name;

            out << '(';

            for(std::size_t pos = 0; pos < functionData.parameters.size(); ++pos) {

                if(pos != 0)
                    out << ", ";

                out << *functionData.parameters[pos];

            }

            out << ')';

            return out;

        }

    };

private:

    FunctionData data;
    std::size_t functionDepth;

    TypePolicy * typePolicy;
    NamePolicy * namePolicy;
    ParamTypePolicy * paramPolicy;
    DeclTypePolicy * declPolicy;

public:

    FunctionPolicy(std::initializer_list<srcSAXEventDispatch::PolicyListener *> listeners)
        : srcSAXEventDispatch::PolicyDispatcher(listeners),
          data{},
          functionDepth(0),
          typePolicy(nullptr),
          namePolicy(nullptr),
          paramPolicy(nullptr),
          declPolicy(nullptr) 
          { 
    
        InitializeFunctionPolicyHandlers();

    }

    ~FunctionPolicy() {

        if(typePolicy)  delete typePolicy;
        if(namePolicy)  delete namePolicy;
        if(paramPolicy) delete paramPolicy;
        if(declPolicy)  delete declPolicy;

    }

protected:
    void * DataInner() const override {

        return new FunctionData(data);

    }

    virtual void Notify(const PolicyDispatcher * policy, const srcSAXEventDispatch::srcSAXEventContext & ctx) override {

        if(typeid(TypePolicy) == typeid(*policy)) {

            data.returnType = policy->Data<TypePolicy::TypeData>();
            ctx.dispatcher->RemoveListenerDispatch(nullptr);

        } else if(typeid(NamePolicy) == typeid(*policy)) {

            data.name = policy->Data<NamePolicy::NameData>(); 
            ctx.dispatcher->RemoveListenerDispatch(nullptr);

        } else if(typeid(ParamTypePolicy) == typeid(*policy)) {

            data.parameters.push_back(policy->Data<ParamTypePolicy::ParamTypeData>()); 
            ctx.dispatcher->RemoveListenerDispatch(nullptr);

        } else if(typeid(DeclTypePolicy) == typeid(*policy)) {

            data.relations.push_back(policy->Data<DeclTypePolicy::DeclTypeData>());
            ctx.dispatcher->RemoveListenerDispatch(nullptr);

        }
    }

private:

    void InitializeFunctionPolicyHandlers() {
        using namespace srcSAXEventDispatch;

        // start of policy
        std::function<void (srcSAXEventContext& ctx)> functionStart = [this](srcSAXEventContext& ctx) {

            if(!functionDepth) {

                functionDepth = ctx.depth;
                data = FunctionData{};

                if(ctx.elementStack.back() == "function" || ctx.elementStack.back() == "function_decl") {

                    if(ctx.isOperator)
                        data.type = OPERATOR;
                    else
                        data.type = FUNCTION;

                } else if(ctx.elementStack.back() == "constructor" || ctx.elementStack.back() == "constructor_decl") {
                    data.type = CONSTRUCTOR;
                } else if(ctx.elementStack.back() == "destructor" || ctx.elementStack.back() == "destructor_decl") {
                    data.type = DESTURCTOR;
                }

                CollectXMLAttributeHandlers();
                CollectTypeHandlers();
                CollectNameHandlers();
                CollectParameterHandlers();
                CollectOtherHandlers();
                CollectDeclstmtHandlers();

            }

        };

        // end of policy
        std::function<void (srcSAXEventContext& ctx)> functionEnd = [this](srcSAXEventContext& ctx) {

            if(functionDepth && functionDepth == ctx.depth) {

                functionDepth = 0;
 
                NotifyAll(ctx);
                InitializeFunctionPolicyHandlers();

            }
           
        };

        openEventMap[ParserState::function] = functionStart;
        openEventMap[ParserState::functiondecl] = functionStart;
        openEventMap[ParserState::constructor] = functionStart;
        openEventMap[ParserState::constructordecl] = functionStart;
        openEventMap[ParserState::destructor] = functionStart;
        openEventMap[ParserState::destructordecl] = functionStart;

        closeEventMap[ParserState::functiondecl] = functionEnd;
        closeEventMap[ParserState::constructordecl] = functionEnd;
        closeEventMap[ParserState::destructordecl] = functionEnd;

        openEventMap[ParserState::functionblock] = [this](srcSAXEventContext& ctx) {

            if(functionDepth && (functionDepth + 1) == ctx.depth) {

                functionDepth = 0;
 
                NotifyAll(ctx);
                InitializeFunctionPolicyHandlers();

            }
           
        };

    }

    void CollectXMLAttributeHandlers() {
        using namespace srcSAXEventDispatch;

        closeEventMap[ParserState::xmlattribute] = [this](srcSAXEventContext& ctx) {

            if(functionDepth == ctx.depth && ctx.currentAttributeName == "stereotype") {

                data.stereotype = ctx.currentAttributeValue;

            }

        };

    }

    /* 
    openEventMap corrilates enums of different types 
    of pieces of a program with a pointer to a lambda
    function in a policy of choice. Then as tags are hit 
    during traversal, those enums are generated and the 
    approriate lambda is called via the map.
    openEventMap for open <> and closeEventMap for close </> 
    */

    /*

    */

    void CollectTypeHandlers() {
        using namespace srcSAXEventDispatch;

        openEventMap[ParserState::type] = [this](srcSAXEventContext& ctx) {

            if(functionDepth && (functionDepth + 1) == ctx.depth) {

                if(!typePolicy) typePolicy = new TypePolicy{this};
                ctx.dispatcher->AddListenerDispatch(typePolicy);

            }

        };

    }

    void CollectNameHandlers() {
        using namespace srcSAXEventDispatch;

        openEventMap[ParserState::name] = [this](srcSAXEventContext& ctx) {

            if(functionDepth && (functionDepth + 1) == ctx.depth) {

                if(!namePolicy) namePolicy = new NamePolicy{this};
                ctx.dispatcher->AddListenerDispatch(namePolicy);

            }

        };

    }


    void CollectParameterHandlers() {
        using namespace srcSAXEventDispatch;

        openEventMap[ParserState::parameterlist] = [this](srcSAXEventContext& ctx) {

            if(functionDepth && (functionDepth + 1) == ctx.depth) {

                openEventMap[ParserState::parameter] = [this](srcSAXEventContext& ctx) {

                    if(functionDepth && (functionDepth + 2) == ctx.depth) {

                        if(!paramPolicy) paramPolicy = new ParamTypePolicy{this};
                        ctx.dispatcher->AddListenerDispatch(paramPolicy);

                    }

                };

            }

        };

        closeEventMap[ParserState::parameterlist] = [this](srcSAXEventContext& ctx) {

            if(functionDepth && (functionDepth + 1) == ctx.depth) {

                NopOpenEvents({ParserState::parameter});

            }

        };

    }


    void CollectOtherHandlers() {
        using namespace srcSAXEventDispatch;

        closeEventMap[ParserState::tokenstring] = [this](srcSAXEventContext& ctx) {

             if(functionDepth && (functionDepth + 1) == ctx.depth) {

                if(ctx.And({ParserState::specifier})) {

                    if(ctx.currentToken == "virtual")
                        data.isVirtual = true;
                    else if(ctx.currentToken == "static")
                        data.isStatic = true;
                    else if(ctx.currentToken == "const")
                        data.isConst = true;
                    else if(ctx.currentToken == "final")
                        data.isFinal = true;
                    else if(ctx.currentToken == "override")
                        data.isOverride = true;
                    else if(ctx.currentToken == "delete")
                        data.isDelete = true;
                    else if(ctx.currentToken == "inline")
                        data.isInline = true;
                    else if(ctx.currentToken == "constexpr")
                        data.isConstExpr = true;

                } else if(ctx.And({ParserState::literal})) {

                    data.isPureVirtual = true;

                }

             }

        };

    }

    /** @todo Will not work with local classes. */
    /** @todo May need to add optimization that ignores declaration statement initialization. */
    void CollectDeclstmtHandlers(){
        using namespace srcSAXEventDispatch;

        openEventMap[ParserState::block] = [this](srcSAXEventContext& ctx) { 

            if(functionDepth && (functionDepth + 1) == ctx.depth) {

                openEventMap[ParserState::declstmt] = [this](srcSAXEventContext& ctx) {
                    if(!declstmtPolicy) declstmtPolicy = new DeclstmtPolicy{this};
                    ctx.dispatcher->AddListenerDispatch(declstmtPolicy);
                }

            }

        };
        
        closeEventMap[ParserState::block] = [this](srcSAXEventContext& ctx) {

            if(functionDepth && (functionDepth + 1) == ctx.depth) {

                NopOpenEvents({ParserState::declstmt});
            }
        };

    }

};

#endif
