import Module from "../lib/siplus_js.js"

export interface FunctionRetriever {
    (value: any, parent: any, ...parameters: any[]): any;
}

export type SIPlusParseOpts = {
    globals?: string[]
}

export type SIPlusInvocationContext = {
    default: any,
    extra?: { [key: string]: any }
}

declare interface M_SIPlusParser {
    parse_interpolated(value: string, opts: SIPlusParseOpts): M_TextConstructor;
    parse_expression(value: string, opts: SIPlusParseOpts): M_ValueRetriever;
    context(): M_ParserContext;
    delete(): void;
}

declare interface M_ParserContext {
    emplace_function(name: string, func: FunctionRetriever): void;
    delete(): void;
}

declare interface M_ValueRetriever {
    retrieve(data: SIPlusInvocationContext): any;
    delete(): void;
}

declare interface M_TextConstructor {
    construct(data: SIPlusInvocationContext): string;
    delete(): void;
}

type SIPlusModule = {
    incrementExceptionRefcount: (e: any) => void,
    decrementExceptionRefcount: (e: any) => void,
    getExceptionMessage: (e: any) => string,
    getCppExceptionThrownObjectFromWebAssemblyException: (e: any) => number,
    siGetExceptionMessage: (e: number) => string,
    getCppExceptionTag: (e: any) => any,
    SIPlus: new () => M_SIPlusParser;
    ValueRetriever: new () => M_ValueRetriever,
    TextConstructor: new () => M_TextConstructor,
    doLeakCheck: () => void
};

var module: SIPlusModule|null = null;

export type SIPlusLibraryOpts = {
    locateFile?: (path: string) => string,
    wasmBinary?: Uint8Array
}

export async function siplus(opts?: SIPlusLibraryOpts): Promise<SIPlus> {
    if(!module) {
        if(opts?.wasmBinary) {
            Module['wasmBinary'] = opts.wasmBinary;
        }

        module = await Module(opts);
    }

    return new SIPlusImpl();
}

export interface TextConstructor {
    construct(data: SIPlusInvocationContext): string;
    delete(): void;
    [Symbol.dispose](): void;
}

export interface ValueRetriever {
    retrieve(data: SIPlusInvocationContext): any;
    delete(): void;
    [Symbol.dispose](): void;
}

export interface SIPlusParserContext {
    emplace_function(name: string, func: FunctionRetriever): void;
    delete(): void;
    [Symbol.dispose](): void;
}

export interface SIPlus {
    parse_interpolation(template: string, opts?: SIPlusParseOpts): TextConstructor;
    parse_expression(expression: string, opts?: SIPlusParseOpts): ValueRetriever;
    context(): SIPlusParserContext;
    delete(): void;
    [Symbol.dispose](): void;
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
            throw new Error(module.siGetExceptionMessage(e));
        } else if(e instanceof (WebAssembly as any).Exception) {
            let ptr = module.getCppExceptionThrownObjectFromWebAssemblyException(e);
            throw new Error(module.siGetExceptionMessage(ptr));
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

    construct(data: SIPlusInvocationContext): string {
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

    retrieve(data: SIPlusInvocationContext): any {
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

    parse_interpolation(template: string, opts?: SIPlusParseOpts): TextConstructor{
        return call(() => {
            return new TextConstructorImpl(this._siplus.parse_interpolated(template, opts ?? {}));
        })
    }

    parse_expression(expression: string, opts?: SIPlusParseOpts): any {
        return call(() => {
            return new ValueRetrieverImpl(this._siplus.parse_expression(expression, opts ?? {}));
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
