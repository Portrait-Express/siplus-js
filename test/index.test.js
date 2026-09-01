const { siplus } = require("../dist/index.js");

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
    var constructor = parser.parse_interpolation(expr);
    try {
        const result = constructor.construct({ default: value });
        expect(result).toEqual(expected);
    } finally {
        constructor.delete();
    }
}

function test_expression(parser, expr, value, expected) {
    var retriever = parser.parse_expression(expr);
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

    siTest("Version 2.0.4", async parser => {
        test_expression(parser, "var $i = 1; [1, 2] | .[$i]", null, 2)
    })

    siTest("Function", parser => {
        var func = (_, parent, toAppend) => {
            if(typeof parent !== "string" || typeof toAppend !== "string") {
                throw Error("Parent or toAppend is not string");
            }

            return parent + toAppend;
        }

        let ctx = parser.context()
        ctx.emplace_function("testAppend", func);
        ctx.delete();

        var retriever = parser.parse_expression(`"Hello, " | testAppend "World"`);
        expect(retriever.retrieve({ default: null })).toEqual("Hello, World");

        retriever.delete();

        test_expression(parser, "@test => ( 1234 ); @test", {}, 1234);
        test_expression(parser, "@test => ( @a => (1234); @a ); @test", {}, 1234);
        test_expression(parser, "@test(val) => ( $val ); @test 1234", {}, 1234);
    })

    siTest("Accessor", parser => {
        let retriever = parser.parse_expression(`.fake`);
        expect(retriever.retrieve({ default: {} })).toEqual(undefined);
        retriever.delete();

        retriever = parser.parse_expression(`.b`);
        expect(retriever.retrieve({ default: { b: 2 } })).toEqual(2);
        retriever.delete();

        retriever = parser.parse_interpolation(`{ .fake }`);
        expect(retriever.construct({ default: {} })).toEqual("");
        retriever.delete();
    });

    siTest("Indexer", parser => {
        test_expression(parser, `.[1]`, [1, 2, 3], 2);
        test_expression(parser, `var $a = 2; .[$a]`, [1, 2, "hello"], "hello");
        test_expression(parser, `var $a = 2; .[($a | sub 1)]`, [1, 2, "hello"], 2);
    });

    siTest("extra", parser => {
        retriever = parser.parse_interpolation(`{ add $job 2 }`, { globals: [ "job" ]});
        expect(retriever.construct({ default: {}, extra: { job: 2 } })).toEqual("4");
        retriever.delete();
    })
})

describe("Stdlib", () => {
    siTest("Converters", parser => {
        test_expression(parser, `eq 9 .`,     "9",  true)
        test_expression(parser, `eq "9" .`,   9,    true)
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

