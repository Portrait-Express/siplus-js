const Module = require("../lib/siplus_js.js");

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
    parseInterpolated(value: string, opts: SIPlusParseOpts): M_TextConstructor;
    parseExpression(value: string, opts: SIPlusParseOpts): M_ValueRetriever;
    context(): M_ParserContext;
    delete(): void;
}

declare interface M_ParserContext {
    addFunction(name: string, func: FunctionRetriever): void;
    delete(): void;
}

declare interface M_ValueRetriever {
    retrieve(data: SIPlusInvocationContext): any;
    retrieveMany(data: SIPlusInvocationContext[]): any[];
    delete(): void;
}

declare interface M_TextConstructor {
    construct(data: SIPlusInvocationContext): string;
    constructMany(data: SIPlusInvocationContext[]): string[];
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

export interface TextConstructor extends Disposable {
    construct(data: SIPlusInvocationContext): string;
    constructMany(data: SIPlusInvocationContext[]): string[];
    delete(): void;
}

export interface ValueRetriever extends Disposable {
    retrieve(data: SIPlusInvocationContext): any;
    retrieveMany(data: SIPlusInvocationContext[]): any[];
    delete(): void;
}

export interface SIPlusParserContext extends Disposable {
    addFunction(name: string, func: FunctionRetriever): void;
    delete(): void;
}

export interface SIPlus extends Disposable {
    parseInterpolation(template: string, opts?: SIPlusParseOpts): TextConstructor;
    parseExpression(expression: string, opts?: SIPlusParseOpts): ValueRetriever;
    context(): SIPlusParserContext;
    delete(): void;
}

/**
 * Utility function to provide similar functionality to 
 * SIPlus::util::getParameters_first_parent.
 *
 * @param {ValueRetriever|null} parent The parent parameter
 * @param {ValueRetriever[]} parameters The parameter list
 * @param {number} count The number of required parameters
 * @param {number} [countOptional] The number of optional parameters. Default: 0
 * @returns {ValueRetriever[]} The parameter list
 */
export function getParametersFirstParent(
    parent: ValueRetriever|null, 
    parameters: ValueRetriever[],
    count: number, countOptional: number = 0
): ValueRetriever[] {
    let ret: ValueRetriever[] = []

    if(parent) {
        ret.push(parent);
    }

    ret.push(...parameters);

    if(ret.length < count) {
        throw RangeError(`Expected at least ${count} parameters. Got ${ret.length}`);
    }
    if(ret.length > count+countOptional) {
        throw RangeError(`Expected at most ${countOptional} parameters. Got ${ret.length}`);
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

    constructMany(data: SIPlusInvocationContext[]): string[] {
        return call(() => {
            return this._constructor.constructMany(data);
        });
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

    /**
     * Invoke a SIPlus ValueRetriever to get a value.
     *
     * @param {SIPlusInvocationContext} data The invocation data
     * @returns {any} The result of the expression.
     */
    retrieve(data: SIPlusInvocationContext): any {
        return call(() => {
            return this._retriever.retrieve(data);
        })
    }

    /**
     * Calls `retrieve` many times in wasm code, reducing the number of times
     * necessary to cross the WASM barrier.
     *
     * The invocation data will be executed in the order provided, and the data
     * returned will be in the same order as the invocations.
     *
     * @param {SIPlusInvocationContext[]} contexts The invocation data.
     * @returns {any} The results of the operations. This is the same length 
     * as `contexts`
     */
    retrieveMany(contexts: SIPlusInvocationContext[]): any[] {
        return call(() => {
            return this._retriever.retrieveMany(contexts);
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

    addFunction(name: string, func: FunctionRetriever): void {
        call(() => {
            this._context.addFunction(name, func);
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
            throw new Error("Not initialized. Call siplusInitialize()");
        }

        this._siplus = new module.SIPlus();
    }

    parseInterpolation(template: string, opts?: SIPlusParseOpts): TextConstructor{
        return call(() => {
            return new TextConstructorImpl(this._siplus.parseInterpolated(template, opts ?? {}));
        })
    }

    parseExpression(expression: string, opts?: SIPlusParseOpts): any {
        return call(() => {
            return new ValueRetrieverImpl(this._siplus.parseExpression(expression, opts ?? {}));
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
