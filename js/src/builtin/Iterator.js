/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

function IteratorIdentity() {
    return this;
}

var LegacyIteratorWrapperMap = new std_WeakMap();

function LegacyIteratorNext(arg) {
    var iter = callFunction(std_WeakMap_get, LegacyIteratorWrapperMap, this);
    try {
        return { value: callContentFunction(iter.next, iter, arg), done: false };
    } catch (e) {
        if (e instanceof std_StopIteration)
            return { value: undefined, done: true };
        throw e;
    }
}

function LegacyIteratorThrow(exn) {
    var iter = callFunction(std_WeakMap_get, LegacyIteratorWrapperMap, this);
    try {
        return { value: callContentFunction(iter.throw, iter, exn), done: false };
    } catch (e) {
        if (e instanceof std_StopIteration)
            return { value: undefined, done: true };
        throw e;
    }
}

function LegacyGeneratorIterator(iter) {
    callFunction(std_WeakMap_set, LegacyIteratorWrapperMap, this, iter);
}

var LegacyIteratorsInitialized = std_Object_create(null);

function InitLegacyIterators() {
    var props = std_Object_create(null);

    props.next = std_Object_create(null);
    props.next.value = LegacyIteratorNext;
    props.next.enumerable = false;
    props.next.configurable = true;
    props.next.writable = true;

    props[std_iterator] = std_Object_create(null);
    props[std_iterator].value = IteratorIdentity;
    props[std_iterator].enumerable = false;
    props[std_iterator].configurable = true;
    props[std_iterator].writable = true;

    props.throw = std_Object_create(null);
    props.throw.value = LegacyIteratorThrow;
    props.throw.enumerable = false;
    props.throw.configurable = true;
    props.throw.writable = true;

    var LegacyGeneratorIteratorProto = std_Object_create(GetIteratorPrototype(), props);
    MakeConstructible(LegacyGeneratorIterator, LegacyGeneratorIteratorProto);

    LegacyIteratorsInitialized.initialized = true;
}

function LegacyGeneratorIteratorShim() {
    var iter = ToObject(this);

    if (!LegacyIteratorsInitialized.initialized)
        InitLegacyIterators();

    return new LegacyGeneratorIterator(iter);
}
