## Possible bugs
- <span style="color:orange;">Annotations don't play nice with moving of pages</span>
- <span style="color:green;"> Check for DIP and NON-DIP calls to the Direct2D API. </span>

## Possible improvements
- <span style="color:orange;">Switch all classes to the Copy and Swap Idiom [(Explanation here)](https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom)</span> 
- <span style="color:orange;">Remove small rectangles from the render queue and replace them with bigger sqaures (Even if this means we have to rerender part of the PDF)</span>
- <span style="color:orange;">Improve the quality of the bezier line (creation _and_ generation).</span>
- <span style="color:green;">Improve the Bezier algorithm to be more efficient.</span>
- <span style="color:green;">Make the Renderer be replacable</span>

--- 
Guide:
<span style="display:flex; flex-direction:row; align-items:center;">
    <span style="display:flex; flex-direction:column; width: 10px; height: 10px; background-color:red;"></span>
    <span style="margin-left: 5px;">High priority</span>
    </span>
<span style="display:flex; flex-direction:row; align-items:center;">
    <span style="display:flex; flex-direction:column; width: 10px; height: 10px; background-color:orange;"></span>
    <span style="margin-left: 5px;">Medium priority</span>
    </span>
<span style="display:flex; flex-direction:row; align-items:center;">
    <span style="display:flex; flex-direction:column; width: 10px; height: 10px; background-color:green;"></span>
    <span style="margin-left: 5px;">Low priority</span>
    </span>

