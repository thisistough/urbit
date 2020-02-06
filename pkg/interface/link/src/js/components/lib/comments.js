import React, { Component } from 'react'
import { CommentItem } from './comment-item';
import { CommentsPagination } from './comments-pagination';

import { uxToHex } from '../../lib/util';
import { api } from '../../api';

export class Comments extends Component {
  constructor(props) {
    super(props);
    this.state = {};
  }

  componentDidMount() {
    let page = this.props.commentPage;
    if (!this.props.comments ||
        !this.props.comments[page] ||
        this.props.comments.local[page]
    ) {
      this.setState({requested: this.props.commentPage});
      api.getCommentsPage(
        this.props.path,
        this.props.url,
        this.props.commentPage);
    }
  }

  render() {
    let props = this.props;

    let page = props.commentPage;

    let commentsObj = !!props.comments
    ? props.comments
    : {};

    let commentsPage = !!commentsObj[page]
    ? commentsObj[page]
    : {};

    let total = !!props.comments
    ? props.comments.totalPages
    : 1;

    let commentsList = Object.keys(commentsPage)
    .map((entry) => {

      let commentObj = commentsPage[entry]
      let { ship, time, udon } = commentObj;

      let members = !!props.members
      ? props.members
      : {};

      let nickname = !!members[ship]
      ? members[ship].nickname
      : "";

      let nameClass = nickname ? "inter" : "mono";

      let color = !!members[ship]
      ? uxToHex(members[ship].color)
      : "000000";

      return(
        <CommentItem
          key={time}
          ship={ship}
          time={time}
          content={udon}
          nickname={nickname}
          nameClass={nameClass}
          color={color}
        />
      )
    })
    return (
      <div>
        {commentsList}
        <CommentsPagination
        key={props.path + props.commentPage}
        path={props.path}
        popout={props.popout}
        linkPage={props.linkPage}
        linkIndex={props.linkIndex}
        url={props.url}
        commentPage={props.commentPage}
        total={total}/>
      </div>
    )
  }
}

export default Comments;