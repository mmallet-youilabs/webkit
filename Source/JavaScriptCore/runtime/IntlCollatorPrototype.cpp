/*
 * Copyright (C) 2015 Andy VanWagoner (andy@vanwagoner.family)
 * Copyright (C) 2016-2019 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IntlCollatorPrototype.h"

#if ENABLE(INTL)

#include "Error.h"
#include "IntlCollator.h"
#include "JSBoundFunction.h"
#include "JSCInlines.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeGetterCompare(JSGlobalObject*, CallFrame*);
static EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeFuncResolvedOptions(JSGlobalObject*, CallFrame*);

}

#include "IntlCollatorPrototype.lut.h"

namespace JSC {

const ClassInfo IntlCollatorPrototype::s_info = { "Object", &Base::s_info, &collatorPrototypeTable, nullptr, CREATE_METHOD_TABLE(IntlCollatorPrototype) };

/* Source for IntlCollatorPrototype.lut.h
@begin collatorPrototypeTable
  compare          IntlCollatorPrototypeGetterCompare        DontEnum|Accessor
  resolvedOptions  IntlCollatorPrototypeFuncResolvedOptions  DontEnum|Function 0
@end
*/

IntlCollatorPrototype* IntlCollatorPrototype::create(VM& vm, JSGlobalObject*, Structure* structure)
{
    IntlCollatorPrototype* object = new (NotNull, allocateCell<IntlCollatorPrototype>(vm.heap)) IntlCollatorPrototype(vm, structure);
    object->finishCreation(vm);
    return object;
}

Structure* IntlCollatorPrototype::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
}

IntlCollatorPrototype::IntlCollatorPrototype(VM& vm, Structure* structure)
    : Base(vm, structure)
{
}

void IntlCollatorPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);

    putDirectWithoutTransition(vm, vm.propertyNames->toStringTagSymbol, jsNontrivialString(vm, "Object"_s), PropertyAttribute::DontEnum | PropertyAttribute::ReadOnly);
}

static EncodedJSValue JSC_HOST_CALL IntlCollatorFuncCompare(JSGlobalObject* globalObject, CallFrame* callFrame)
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    // 10.3.4 Collator Compare Functions (ECMA-402 2.0)
    // 1. Let collator be the this value.
    // 2. Assert: Type(collator) is Object and collator has an [[initializedCollator]] internal slot whose value is true.
    IntlCollator* collator = jsCast<IntlCollator*>(callFrame->thisValue());

    // 3. If x is not provided, let x be undefined.
    // 4. If y is not provided, let y be undefined.
    // 5. Let X be ToString(x).
    JSString* x = callFrame->argument(0).toString(globalObject);
    // 6. ReturnIfAbrupt(X).
    RETURN_IF_EXCEPTION(scope, encodedJSValue());

    // 7. Let Y be ToString(y).
    JSString* y = callFrame->argument(1).toString(globalObject);
    // 8. ReturnIfAbrupt(Y).
    RETURN_IF_EXCEPTION(scope, encodedJSValue());

    // 9. Return CompareStrings(collator, X, Y).
    auto xViewWithString = x->viewWithUnderlyingString(globalObject);
    RETURN_IF_EXCEPTION(scope, encodedJSValue());
    auto yViewWithString = y->viewWithUnderlyingString(globalObject);
    RETURN_IF_EXCEPTION(scope, encodedJSValue());
    RELEASE_AND_RETURN(scope, JSValue::encode(collator->compareStrings(globalObject, xViewWithString.view, yViewWithString.view)));
}

EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeGetterCompare(JSGlobalObject* globalObject, CallFrame* callFrame)
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    // 10.3.3 Intl.Collator.prototype.compare (ECMA-402 2.0)
    // 1. Let collator be this Collator object.
    IntlCollator* collator = jsDynamicCast<IntlCollator*>(vm, callFrame->thisValue());
    if (!collator)
        return JSValue::encode(throwTypeError(globalObject, scope, "Intl.Collator.prototype.compare called on value that's not an object initialized as a Collator"_s));

    JSBoundFunction* boundCompare = collator->boundCompare();
    // 2. If collator.[[boundCompare]] is undefined,
    if (!boundCompare) {
        JSGlobalObject* globalObject = collator->globalObject(vm);
        // a. Let F be a new built-in function object as defined in 11.3.4.
        // b. The value of F’s length property is 2.
        JSFunction* targetObject = JSFunction::create(vm, globalObject, 2, "compare"_s, IntlCollatorFuncCompare, NoIntrinsic);

        // c. Let bc be BoundFunctionCreate(F, «this value»).
        boundCompare = JSBoundFunction::create(vm, globalObject, targetObject, collator, nullptr, 2, nullptr);
        RETURN_IF_EXCEPTION(scope, encodedJSValue());
        // d. Set collator.[[boundCompare]] to bc.
        collator->setBoundCompare(vm, boundCompare);
    }
    // 3. Return collator.[[boundCompare]].
    return JSValue::encode(boundCompare);
}

EncodedJSValue JSC_HOST_CALL IntlCollatorPrototypeFuncResolvedOptions(JSGlobalObject* globalObject, CallFrame* callFrame)
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);

    // 10.3.5 Intl.Collator.prototype.resolvedOptions() (ECMA-402 2.0)
    IntlCollator* collator = jsDynamicCast<IntlCollator*>(vm, callFrame->thisValue());
    if (!collator)
        return JSValue::encode(throwTypeError(globalObject, scope, "Intl.Collator.prototype.resolvedOptions called on value that's not an object initialized as a Collator"_s));

    RELEASE_AND_RETURN(scope, JSValue::encode(collator->resolvedOptions(globalObject)));
}

} // namespace JSC

#endif // ENABLE(INTL)
