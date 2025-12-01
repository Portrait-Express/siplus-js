import Module from "../lib/siplus_js.js"

export interface FunctionRetriever {
    (value: any, parent: any, ...parameters: any[]): any;
}

declare interface M_SIPlusParser {
    parse_interpolated(value: any): M_TextConstructor;
    parse_expression(value: any): M_ValueRetriever;
    context(): M_ParserContext;
    delete(): void;
}

declare interface M_ParserContext {
    emplace_function(name: string, func: FunctionRetriever): void;
    delete(): void;
}

declare interface M_ValueRetriever {
    retrieve(value: any): any;
    delete(): void;
}

declare interface M_TextConstructor {
    construct(value: any): string;
    delete(): void;
}

type SIPlusModule = {
    SIPlus: new () => M_SIPlusParser;
    ValueRetriever: new () => M_ValueRetriever,
    TextConstructor: new () => M_TextConstructor,
    getExceptionMessage: (e: number) => string,
    doLeakCheck: () => void
};

var module: SIPlusModule|null = null;

export type SIPlusLibraryOpts = {
    locateFile?: (path: string) => string
}

export async function siplus(opts: SIPlusLibraryOpts): Promise<SIPlus> {
    if(!module) {
        module = await Module(opts);
    }

    return new SIPlusImpl();
}

export interface TextConstructor {
    construct(data: any): string;
    delete(): void;
    [Symbol.dispose](): void;
}

export interface ValueRetriever {
    retrieve(data: any): any;
    delete(): void;
    [Symbol.dispose](): void;
}

export interface SIPlusParserContext {
    emplace_function(name: string, func: FunctionRetriever): void;
    delete(): void;
    [Symbol.dispose](): void;
}

export interface SIPlus {
    parse_interpolation(template: string): TextConstructor;
    parse_expression(expression: string): any;
    context(): SIPlusParserContext;
    delete(): void;
    [Symbol.dispose](): void;
}

export function getExceptionMessage(e: number) {
    return module.getExceptionMessage(e);
}

export function doLeakCheck() {
    return module.doLeakCheck();
}

/**
 * Utility function to provide similar functionality to 
 * SIPlus::util::get_parameters_first_parent.
 *
 * @param {ValueRetriever|null} parent The parent parameter
 * @param {ValueRetriever[]} parameters The parameter list
 * @param {number} count The number of required parameters
 * @param {number} [count_optional] The number of optional parameters. Default: 0
 * @returns {ValueRetriever[]} The parameter list
 */
export function getParametersFirstParent(
    parent: ValueRetriever|null, 
    parameters: ValueRetriever[],
    count: number, count_optional: number = 0
): ValueRetriever[] {
    let ret: ValueRetriever[] = []

    if(parent) {
        ret.push(parent);
    }

    ret.push(...parameters);

    if(ret.length < count) {
        throw RangeError(`Expected at least ${count} parameters. Got ${ret.length}`);
    }
    if(ret.length > count+count_optional) {
        throw RangeError(`Expected at most ${count_optional} parameters. Got ${ret.length}`);
    }

    return ret;
}

function call<T>(c: () => T): T {
    try {
        return c();
    } catch(e) {
        if(typeof e === 'number') {
            throw new Error(getExceptionMessage(e));
        } else {
            throw e;
        }
    }
}

class TextConstructorImpl {
    private _constructor: M_TextConstructor;

    constructor(text: M_TextConstructor) {
        this._constructor = text;
    }

    construct(data: any): string {
        return call(() => {
            return this._constructor.construct(data);
        })
    }

    delete() {
        return call(() => {
            this._constructor.delete();
        });
    }

    [Symbol.dispose]() {
        this.delete();
    }
}

class ValueRetrieverImpl implements ValueRetriever {
    private _retriever: M_ValueRetriever;

    constructor(retriever: M_ValueRetriever) {
        this._retriever = retriever;
    }

    retrieve(data: any): any {
        return call(() => {
            return this._retriever.retrieve(data);
        })
    }

    delete() {
        return call(() => {
            this._retriever.delete();
        });
    }

    [Symbol.dispose]() {
        this.delete();
    }
}

class SIPlusParserContextImpl implements SIPlusParserContext {
    private _context: M_ParserContext;

    constructor(impl: M_ParserContext) {
        this._context = impl;
    }

    emplace_function(name: string, func: FunctionRetriever): void {
        call(() => {
            this._context.emplace_function(name, func);
        });
    }

    delete() {
        call(() => {
            this._context.delete();
        })
    }

    [Symbol.dispose]() {
        this.delete();
    }
}

class SIPlusImpl implements SIPlus {
    private _siplus: M_SIPlusParser;

    constructor() {
        if(!module) {
            throw new Error("Not initialized. Call siplus_initialize()");
        }

        this._siplus = new module.SIPlus();
    }

    parse_interpolation(template: string): TextConstructor{
        return call(() => {
            return new TextConstructorImpl(this._siplus.parse_interpolated(template));
        })
    }

    parse_expression(expression: string): any {
        return call(() => {
            return new ValueRetrieverImpl(this._siplus.parse_expression(expression));
        })
    }

    context() {
        return call(() => {
            return new SIPlusParserContextImpl(this._siplus.context());
        })
    }

    delete() {
        return call(() => {
           this._siplus.delete();
        });
    }

    [Symbol.dispose]() {
        this.delete();
    }
}
