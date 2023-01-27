package com.samsung.lwe.escargot;

import android.content.Context;

import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.ext.junit.runners.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

import java.util.Optional;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class ExampleInstrumentedTest {
    @Test
    public void simpleRunTest() {
        Globals.initializeGlobals();

        Memory.setGCFrequency(24);

        // pass if there is no crash
        VMInstance.create(Optional.empty(), Optional.empty());
        VMInstance.create(Optional.of("en-US"), Optional.empty());
        VMInstance.create(Optional.empty(), Optional.of("Asia/Seoul"));

        VMInstance vmInstance = VMInstance.create(Optional.of("en-US"), Optional.of("Asia/Seoul"));
        assertTrue(vmInstance.hasValidNativePointer());
        com.samsung.lwe.escargot.Context context = com.samsung.lwe.escargot.Context.create(vmInstance);
        assertTrue(context.hasValidNativePointer());

        // test script parsing error
        assertFalse(Evaluator.evalScript(context, "@@", "invalid", true).isPresent());
        // test runtime error
        assertFalse(Evaluator.evalScript(context, "throw new Error()", "invalid", true).isPresent());

        assertTrue(Evaluator.evalScript(context, "a = 1", "from_java.js", true).get().equals("1"));
        assertTrue(Evaluator.evalScript(context, "a", "from_java2.js", true).get().equals("1"));
        assertTrue(Evaluator.evalScript(context, "a = new Date('2000/1/1')", "from_java3.js", true).get().contains("2000"));

        // test ICU
        assertTrue(Evaluator.evalScript(context,
                "const koDtf = new Intl.DateTimeFormat(\"ko\", { dateStyle: \"long\" }); koDtf.format(a)", "from_java4.js", true).get().contains("2000"));

        class TestBridge extends Bridge.Adapter {
            public boolean called = false;
            @Override
            public Optional<String> callback(Optional<String> data) {
                assertFalse(called);
                assertTrue(data.get().equals("dddd"));
                called = true;
                return Optional.of(data.get() + "ASdfasdfasdf");
            }
        };
        TestBridge testBridge = new TestBridge();
        Bridge.register(context, "Native", "addString", testBridge);
        Bridge.register(context, "Native", "returnString", new Bridge.Adapter() {
            @Override
            public Optional<String> callback(Optional<String> data) {
                assertFalse(data.isPresent());
                return Optional.of("string from java");
            }
        });
        Bridge.register(context, "Native", "returnNothing", new Bridge.Adapter() {
            @Override
            public Optional<String> callback(Optional<String> data) {
                return Optional.empty();
            }
        });

        assertTrue(Evaluator.evalScript(context, "Native.addString('dddd')", "from_java5.js", true).get().equals("ddddASdfasdfasdf"));
        assertTrue(testBridge.called);

        assertTrue(Evaluator.evalScript(context, "Native.returnString()", "from_java6.js", true).get().equals("string from java"));
        assertTrue(Evaluator.evalScript(context, "Native.returnNothing() === undefined", "from_java7.js", true).get().equals("true"));

        context.destroy();
        // can call twice
        context.destroy();
        assertFalse(context.hasValidNativePointer());
        vmInstance.destroy();
        assertFalse(vmInstance.hasValidNativePointer());
        Globals.finalizeGlobals();
    }
}