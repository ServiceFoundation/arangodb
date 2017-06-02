/* -*- Mode: java; tab-width: 8 -*-
 * Copyright (C) 1997, 1998 Netscape Communications Corporation,
 * All Rights Reserved.
 */

package com.netscape.javascript.qa.drivers;

import java.io.*;
import java.util.StringTokenizer;

/**
 *  RefEnv is used to run a JavaScript test in the JavaScript engine
 *  implemented in C.  RefEnv creates a new ObservedTask and processes
 *  the standard output and standard error of the tests to determine the 
 *  test result. The RefDrv expects the process output to contain the 
 *  following content:
 *
 *  <p>
 *  1 - anything (before the formatted output can be unstructured.
 *
 *  <p>
 *
 *  2 - the number of test cases output in the following way.
 *  <pre>
 *    <#TEST CASES SIZE>
 *    int
 *  </pre>
 *
 *  3 - the results of each test formatted in the following way
 *      (name, expected, actual, description, and reason can all be multilines)
 *  <pre>
 *    <#TEST CASE PASSED>
 *    true|false
 *    <#TEST CASE NAME>
 *    text
 *    <#TEST CASE EXPECTED>
 *    text
 *    <#TEST CASE ACTUAL>
 *    text
 *    <#TEST CASE DESCRIPTION>
 *    text
 *    <#TEST CASE REASON>
 *    text
 *    <#TEST CASE BUGNUMBER>
 *    text
 *  </pre>
 *
 *  4 - a marker that signifies the end of the tests
 *
 *  <pre>
 *    <#TEST CASES DONE>
 *  </pre>
 
 *  5 - after the end of the tests you can have more unstructed text.
 *  <p>
 *  The test case output is generated by the stopTest function, which is in 
 *  the shared functions file, jsref.js.
 *
 *  @see com.netscape.javascript.qa.drivers.TestEnvironment
 *  @see ObservedTask 
 *
 *  @author christine@netscape.com
 *  @author Nick Lerissa
 */
public class RefEnv implements TestEnvironment {
    TestFile    file;
    TestSuite   suite;
    RefDrv  driver;
    ObservedTask task;
    
    /**
     *  Create a new RefEnv.
     */
    public RefEnv( TestFile f, TestSuite s, RefDrv d) {
        this.file = f;
        this.suite = s;
        this.driver = d;
    }

    /**
     *   Called by the driver to execute the test program.
     */
    public void runTest() {
        driver.p( file.name );
        try {
            file.startTime = driver.getCurrentTime();
            createContext();
            executeTestFile();
            file.endTime = driver.getCurrentTime();

            if (task.getExitValue() != 0) {
                if ( file.name.endsWith( "-n.js" )) {
                    file.passed = true;
                } else {                    
                    suite.passed   = false;
                    file.passed    = false;
                }                
            }
            if ( ! parseResult() ) {
                if ( file.name.endsWith( "-n.js" ) ) {
                    file.passed = true; 
                } else {                    
                    suite.passed   = false;
                    file.passed    = false;
                }                    
                file.exception = new String(task.getError());                
            }

        } catch ( Exception e ) {
            suite.passed = false;
            file.passed  = false;
            file.exception = "Unknown process exception.";
/*            
            file.exception =  new String(task.getError())
                              + " exit value: " + task.getExitValue()
                              + "\nThrew Exception:" + e;
*/                              
        }
    }

    /**
     *  Instantiate a new JavaScript shell, passing the TestFile as an
     *  argument.
     *
     */
    public Object createContext() {
        if ( driver.CODE_COVERAGE ) {
            String command = "coverage " +
                "/SaveMergeData /SaveMergeTextData "+
                driver.EXECUTABLE + " -f " + 
                driver.HELPER_FUNCTIONS.getAbsolutePath() + " -f " + 
                file.filePath;
            
            System.out.println( "command is " + command );
            
            task = new ObservedTask( command, this );
        } else {
            task = new ObservedTask( driver.EXECUTABLE + " -f " + 
                driver.HELPER_FUNCTIONS.getAbsolutePath() + " -f " + 
                file.filePath, this);
        }                
        return (Object) task;                
    }
    /**
     *  Start the shell process.
     *
     */
    public Object executeTestFile() {
        try {
            task.exec();
        }   catch ( IOException e ) {
            driver.p( e.toString() );
            file.exception = e.toString();
            e.printStackTrace();
        }            
        return null;
    }

    /**
     *  Parse the standard output of the process, and try to create new 
     *  TestCase objects.
     */
    public boolean parseResult() {
        String line;
        int i,j;

        BufferedReader br = new BufferedReader(new StringReader(
            new String(task.getInput())));
        try {
            do {
                line = br.readLine();
                driver.p( line );
                
                if (line == null) {
                    driver.p("\tERROR: No lines to read");
                    return false;
                }
            } while (!line.equals(sizeTag));
    
            if ((line = br.readLine()) == null) 
                return false;
            
            file.totalCases = Integer.valueOf(line).intValue();

            if ((line = br.readLine()) == null) {
                driver.p("\tERROR: No lines after " + sizeTag);
                return false;
            }
            
        } catch (NumberFormatException nfe) {
            driver.p("\tERROR: No integer after " + sizeTag);
            return false;
        } catch ( IOException e ) {
            driver.p( "Exception reading process output:" + e.toString() );
            file.exception  = e.toString();
            return false;
        }            
        
        for ( i = 0; i < file.totalCases; i++) {
            String values[] = new String[tags.length];
            try {
                for ( j = 0; j < tags.length; j++) {
                    values[j] = null;
                
                    if (!line.startsWith(tags[j])) {
                        driver.p("line didn't start with " + tags[j] +":"+line);
                        return false;
                    }                    
                    while (((line = br.readLine()) != null) && 
                        (!(line.startsWith(startTag))))
                    {
                        values[j] = (values[j] == null) ? line : (values[j] + 
                            "\n" + line);
                    }
                    if (values[j] == null) values[j] = "";
                }
                if ((line == null) && (i < file.totalCases - 1)) {
                    driver.p("line == null and " + i + "<" +
                        (file.totalCases - 1));
                    return false;
                }                
            } catch ( IOException e ) {
                driver.p( "Exception reading process output: " + e );
                file.exception = e.toString();
                return false;
            }                
                
            TestCase rt = new TestCase(values[0],values[1],values[4],values[2],
                values[3],values[5]);
                
            file.bugnumber = values[6];
            file.caseVector.addElement( rt );

            if ( rt.passed.equals("false") ) {
                if ( file.name.endsWith( "-n.js" ) ) {
                    this.file.passed = true;
                } else {                    
                    this.file.passed  = false;
                    this.suite.passed = false;
                }                    
            }

        }
        
        if ( file.totalCases == 0 ) {
            if ( file.name.endsWith( "-n.js" ) ) {
                this.file.passed = true;
            } else {                    
                this.file.reason  = "File contains no testcases. " + this.file.reason;
                this.file.passed  = false;
                this.suite.passed = false;
            }                    
        }
        
        return true;
    }
    /**
     *  Method required by TestEnvironment; this implemenation does nothing.  
     *
     */
    public void close(){
        return;
    }

    /**
     * array of in output file to get attributes of a specific TestCase
     */
    public static final String tags[];
    /**
     * tag to specify the number of TestCases
     */
    public static final String sizeTag  = "<#TEST CASES SIZE>";
    /**
     * beginning of a tag
     */
    public static final String startTag = "<#TEST CASE";
    /**
     * end of a tag
     */
    public static final String endTag   = ">";

    /**
     * creating of String tags[]
     */
    static {
        String fields[] = { "PASSED", "NAME", "EXPECTED", "ACTUAL", "DESCRIPTION", "REASON",
                            "BUGNUMBER" };

        tags = new String[fields.length];

        for (int i = 0; i < fields.length; ++i) {
            tags[i] = startTag + " " + fields[i] + endTag;
        }
    }    
}