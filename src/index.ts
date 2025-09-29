import Module from "../lib/siplus_js.js"

declare interface M_SIPlusParser {
    parse_interpolated(value: any): M_TextConstructor;
    parse_expression(value: any): M_ValueRetriever;
    delete(): void;
}

declare interface M_ParserContext {
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
    ParserContext: new () => M_ParserContext,
    ValueRetriever: new () => M_ValueRetriever,
    TextConstructor: new () => M_TextConstructor,
    getExceptionMessage: (e: number) => string,
    doLeakCheck: () => void
};

var module: SIPlusModule|null = null;

export type SIPlusLibraryOpts = {
    locateFile?: (path: string) => string
}

export async function siplus_init(opts: SIPlusLibraryOpts) {
    if(!module) {
        module = await Module(opts);
    }
}

export function siplus_free() {
    if(!module) return;
    module = null;
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

export class TextConstructor {
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
}

export class ValueRetriever {
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
}

export class SIPlus {
    private _siplus: M_SIPlusParser;

    constructor() {
        if(!module) {
            throw new Error("Not initialized. Call siplus_initialize()");
        }

        this._siplus = new module.SIPlus();
    }

    parse_interpolation(template: string): TextConstructor {
        return call(() => {
            return new TextConstructor(this._siplus.parse_interpolated(template));
        })
    }

    parse_expression(expression: string): any {
        return call(() => {
            return new ValueRetriever(this._siplus.parse_expression(expression));
        })
    }

    delete() {
        return call(() => {
           this._siplus.delete();
        });
    }
}

export function getExceptionMessage(e: number) {
    return module.getExceptionMessage(e);
}

export function doLeakCheck() {
    return module.doLeakCheck();
}
