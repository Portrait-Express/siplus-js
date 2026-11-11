const { siplus } = require("../dist/index.js");

/** @import { describe, test, it, expect } from "@jest/globals" */

/**
 * @typedef {(parser: import("../types/index.js").SIPlus) => void|Promise<void>} SITestCallback
 */

/**
 * Test involvoing a parser
 *
 * @param {string} name The name of the test.
 * @param {SITestCallback} cb The test implementation
 */
function siTest(name, cb) {
    it(name, async () => {
        const parser = await siplus();

        try {
            await cb(parser);
        } finally {
            parser.delete();
        }
    })
}

function test_interpolation(parser, expr, value, expected) {
    var constructor = parser.parseInterpolation(expr);
    try {
        const result = constructor.construct({ default: value });
        expect(result).toEqual(expected);
    } finally {
        constructor.delete();
    }
}

function test_expression(parser, expr, value, expected) {
    var retriever = parser.parseExpression(expr);
    try {
        const result = retriever.retrieve({ default: value });
        if(typeof expected === 'function') {
            if(!expected(result)) {
                throw new Error(`Expression "${expr}" failed. Result test function failed.`)
            }
        } else {
            expect(result).toEqual(expected);
        }
    } finally {
        retriever.delete();
    }
}

describe('SIPlus Tests', () => {
    siTest("Parser", parser => {
        test_interpolation(parser, "Hello { .id }", {id: 1}, "Hello 1")
        test_expression(parser, "map . .id", [{id: 1}, {id: 2}], [1, 2])
    })

    siTest("Version 2.3.1", async parser => {
        test_expression(parser, "[1, 2, 3] | last", null, 3);
    })

    siTest("Function", parser => {
        var func = (_, parent, toAppend) => {
            if(typeof parent !== "string" || typeof toAppend !== "string") {
                throw Error("Parent or toAppend is not string");
            }

            return parent + toAppend;
        }

        let ctx = parser.context()
        ctx.addFunction("testAppend", func);
        ctx.delete();

        var retriever = parser.parseExpression(`"Hello, " | testAppend "World"`);
        expect(retriever.retrieve({ default: null })).toEqual("Hello, World");

        retriever.delete();

        test_expression(parser, "@test => ( 1234 ); @test", {}, 1234);
        test_expression(parser, "@test => ( @a => (1234); @a ); @test", {}, 1234);
        test_expression(parser, "@test(val) => ( $val ); @test 1234", {}, 1234);
    })

    siTest("Accessor", parser => {
        let retriever = parser.parseExpression(`.fake`);
        expect(retriever.retrieve({ default: {} })).toEqual(null);
        retriever.delete();

        retriever = parser.parseExpression(`.b`);
        expect(retriever.retrieve({ default: { b: 2 } })).toEqual(2);
        retriever.delete();

        retriever = parser.parseInterpolation(`{ .fake }`);
        expect(retriever.construct({ default: {} })).toEqual("");
        retriever.delete();
    });

    siTest("Indexer", parser => {
        test_expression(parser, `.[1]`, [1, 2, 3], 2);
        test_expression(parser, `var $a = 2; .[$a]`, [1, 2, "hello"], "hello");
        test_expression(parser, `var $a = 2; .[($a | sub 1)]`, [1, 2, "hello"], 2);
    });

    siTest("extra", parser => {
        retriever = parser.parseInterpolation(`{ add $job 2 }`, { globals: [ "job" ]});
        expect(retriever.construct({ default: {}, extra: { job: 2 } })).toEqual("4");
        retriever.delete();
    })

    siTest("ValueRetriever.retrieveMany", parser => {
        const retriever = parser.parseExpression("rand"); 
        try {
            const results = retriever.retrieveMany(new Array(24).fill({ default: null }));

            expect(results).toEqual(
                results.map(() => expect.any(Number))
            );
        } finally {
            retriever.delete();
        }
    })

    siTest("TextContructor.constructMany", parser => {
        const retriever = parser.parseInterpolation("Hello, { rand }"); 
        try {
            const results = retriever.constructMany(new Array(24).fill({ default: null }));

            expect(results).toEqual(
                results.map(() => expect.any(String))
            );
        } finally {
            retriever.delete();
        }
    })
})

describe("Stdlib", () => {
    siTest("Converters", parser => {
        test_expression(parser, `eq 9 .`,      9,    true)
        test_expression(parser, `eq "9" .`,    "9",  true)
        test_expression(parser, `and true .`,  true, true)
        test_expression(parser, `and false .`, true, false)
        test_expression(parser, `[1, 2, 3] | map (add . 2)`, null, [3, 4, 5])
    })

    siTest("split", parser => {
        test_expression(parser, `"1,2,3,4,5" | split ","`, null, ["1", "2", "3", "4", "5"])
    })

    siTest("rand", parser => {
        test_expression(parser, `rand`, null, v => v >= 0 && v <= 1);
        test_expression(parser, `rand 0 10`, null, v => v >= 0 && v <= 10);
    });

    siTest("set", parser => {
        test_expression(parser, `set_new | set_add 2 | set_has 2`, null, true);
        test_expression(parser, `set_new | set_add 20 | set_has 2`, null, false);
    })
});

describe("Issues", () => {
    siTest("#1: RuntimeError on no piped value", parser => {
        function dummy() { return null; }

        const ctx = parser.context();
        ctx.addFunction("dummy", dummy);
        ctx.delete();

        test_expression(parser, `dummy`, null, null);
    });
})
