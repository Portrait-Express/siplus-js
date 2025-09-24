import Module from "../lib/siplus_js.js"

declare interface M_SIPlusParser {
    parse_interpolated(value: any): TextConstructor;
    parse_expression(value: any): ValueRetriever;
}

declare interface M_ParserContext {

}

declare interface M_ValueRetriever {
    retrieve(value: any): any;
}

declare interface M_TextConstructor {
    construct(value: any): string;
}

type SIPlusModule = {
    SIPlus: new() => M_SIPlusParser;
    ParserContext: new() => M_ParserContext,
    ValueRetriever: new() => M_ValueRetriever,
    TextConstructor: new() => M_TextConstructor,
    getExceptionMessage: (e: number) => string
};

const module: SIPlusModule = await Module();

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
}

export class SIPlus {
    private _siplus: M_SIPlusParser;

    constructor() {
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
}

export function getExceptionMessage(e: number) {
    return module.getExceptionMessage(e);
}
